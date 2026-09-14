#include "bos/png.hpp"
#include "bos/resource.hpp"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

namespace bos {

namespace {

// ============================================================================
// Internal RFC 1951 Deflate Decompressor
// ============================================================================
#define MAX_BITS 16
#define MAX_LIT  288
#define MAX_DIST 32
#define MAX_CODE 19

struct HuffmanTree {
    uint16_t counts[MAX_BITS];
    uint16_t symbols[MAX_LIT];
    int max_sym;
};

struct InflateState {
    const uint8_t* in;
    uint32_t in_size;
    uint32_t in_pos;
    uint32_t bit_buf;
    int bit_cnt;

    uint8_t* out;
    uint32_t out_cap;
    uint32_t out_pos;
};

static int read_bits(InflateState* s, int n) {
    while (s->bit_cnt < n) {
        if (s->in_pos >= s->in_size) return -1;
        s->bit_buf |= ((uint32_t)s->in[s->in_pos++] << s->bit_cnt);
        s->bit_cnt += 8;
    }
    int res = (int)(s->bit_buf & ((1U << n) - 1));
    s->bit_buf >>= n;
    s->bit_cnt -= n;
    return res;
}

static int build_tree(HuffmanTree* t, const uint8_t* lengths, int num) {
    uint16_t offs[MAX_BITS];
    for (int i = 0; i < MAX_BITS; i++) t->counts[i] = 0;
    for (int i = 0; i < num; i++) t->counts[lengths[i]]++;
    t->counts[0] = 0;

    offs[1] = 0;
    for (int i = 1; i < MAX_BITS - 1; i++) offs[i + 1] = offs[i] + t->counts[i];

    for (int i = 0; i < num; i++) {
        if (lengths[i]) t->symbols[offs[lengths[i]]++] = (uint16_t)i;
    }
    t->max_sym = num;
    return 0;
}

static int decode_symbol(InflateState* s, const HuffmanTree* t) {
    int sum = 0, cur = 0, len = 0;
    do {
        int bit = read_bits(s, 1);
        if (bit < 0) return -1;
        cur = (cur << 1) | bit;
        len++;
        if (len >= MAX_BITS) return -1;
        sum += t->counts[len];
        cur -= t->counts[len];
    } while (cur >= 0);
    if (sum + cur < 0 || sum + cur >= MAX_LIT) return -1;
    return t->symbols[sum + cur];
}

static int inflate_uncompressed(InflateState* s) {
    s->bit_buf = 0;
    s->bit_cnt = 0;
    if (s->in_pos + 4 > s->in_size) return -1;
    uint16_t len  = (uint16_t)(s->in[s->in_pos] | (s->in[s->in_pos + 1] << 8));
    uint16_t nlen = (uint16_t)(s->in[s->in_pos + 2] | (s->in[s->in_pos + 3] << 8));
    s->in_pos += 4;
    if (len != (uint16_t)~nlen) return -1;
    if (s->in_pos + len > s->in_size || s->out_pos + len > s->out_cap) return -1;
    for (int i = 0; i < len; i++) {
        s->out[s->out_pos++] = s->in[s->in_pos++];
    }
    return 0;
}

static const uint16_t kLengthBase[29] = {
    3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31, 35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258
};
static const uint8_t kLengthExtra[29] = {
    0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0
};
static const uint16_t kDistBase[30] = {
    1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193, 257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577
};
static const uint8_t kDistExtra[30] = {
    0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13
};

static int inflate_huffman(InflateState* s, const HuffmanTree* lt, const HuffmanTree* dt) {
    while (true) {
        int sym = decode_symbol(s, lt);
        if (sym < 0) return -1;
        if (sym < 256) {
            if (s->out_pos >= s->out_cap) return -1;
            s->out[s->out_pos++] = (uint8_t)sym;
        } else if (sym == 256) {
            break;
        } else {
            sym -= 257;
            if (sym >= 29) return -1;
            int ext = kLengthExtra[sym];
            int ext_bits = ext ? read_bits(s, ext) : 0;
            if (ext_bits < 0) return -1;
            int len = kLengthBase[sym] + ext_bits;

            int dist_sym = decode_symbol(s, dt);
            if (dist_sym < 0 || dist_sym >= 30) return -1;
            ext = kDistExtra[dist_sym];
            ext_bits = ext ? read_bits(s, ext) : 0;
            if (ext_bits < 0) return -1;
            int dist = kDistBase[dist_sym] + ext_bits;

            if (s->out_pos + len > s->out_cap || dist > (int)s->out_pos) return -1;
            for (int i = 0; i < len; i++) {
                s->out[s->out_pos] = s->out[s->out_pos - dist];
                s->out_pos++;
            }
        }
    }
    return 0;
}

static int inflate_block(InflateState* s) {
    int bfinal = read_bits(s, 1);
    int btype = read_bits(s, 2);
    if (bfinal < 0 || btype < 0) return -1;

    if (btype == 0) {
        if (inflate_uncompressed(s) < 0) return -1;
    } else if (btype == 1 || btype == 2) {
        HuffmanTree lt, dt;
        if (btype == 1) {
            uint8_t lens[288];
            for (int i = 0; i < 144; i++) lens[i] = 8;
            for (int i = 144; i < 256; i++) lens[i] = 9;
            for (int i = 256; i < 280; i++) lens[i] = 7;
            for (int i = 280; i < 288; i++) lens[i] = 8;
            build_tree(&lt, lens, 288);

            uint8_t dlens[32];
            for (int i = 0; i < 32; i++) dlens[i] = 5;
            build_tree(&dt, dlens, 32);
        } else {
            int hlit_raw = read_bits(s, 5);
            if (hlit_raw < 0) return -1;
            int hlit = hlit_raw + 257;

            int hdist_raw = read_bits(s, 5);
            if (hdist_raw < 0) return -1;
            int hdist = hdist_raw + 1;

            int hclen_raw = read_bits(s, 4);
            if (hclen_raw < 0) return -1;
            int hclen = hclen_raw + 4;

            static const uint8_t clen_order[19] = {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};
            uint8_t clens[19] = {0};
            for (int i = 0; i < hclen; i++) {
                int cb = read_bits(s, 3);
                if (cb < 0) return -1;
                clens[clen_order[i]] = (uint8_t)cb;
            }

            HuffmanTree ct;
            build_tree(&ct, clens, 19);

            uint8_t lens[MAX_LIT + MAX_DIST] = {0};
            int num = hlit + hdist;
            if (num > MAX_LIT + MAX_DIST) return -1;

            int i = 0;
            while (i < num) {
                int sym = decode_symbol(s, &ct);
                if (sym < 0) return -1;
                if (sym < 16) {
                    lens[i++] = (uint8_t)sym;
                } else {
                    int rep = 0;
                    uint8_t val = 0;
                    if (sym == 16) {
                        int r = read_bits(s, 2);
                        if (r < 0 || i == 0) return -1;
                        rep = r + 3;
                        val = lens[i - 1];
                    } else if (sym == 17) {
                        int r = read_bits(s, 3);
                        if (r < 0) return -1;
                        rep = r + 3;
                    } else if (sym == 18) {
                        int r = read_bits(s, 7);
                        if (r < 0) return -1;
                        rep = r + 11;
                    }
                    if (i + rep > num) return -1;
                    while (rep--) lens[i++] = val;
                }
            }
            build_tree(&lt, lens, hlit);
            build_tree(&dt, lens + hlit, hdist);
        }
        if (inflate_huffman(s, &lt, &dt) < 0) return -1;
    } else {
        return -1;
    }
    return bfinal;
}

static int inflate_decompress(const uint8_t* in_data, uint32_t in_size, uint8_t* out_data, uint32_t out_capacity, uint32_t* out_size) {
    if (!in_data || !out_data || !out_size) return -1;
    if (in_size < 6) return -1;

    // Check zlib header
    uint8_t cmf = in_data[0];
    if ((cmf & 0x0F) != 8) return -1;

    InflateState s = {};
    s.in = in_data + 2;
    s.in_size = in_size - 6; // exclude 2 header bytes + 4 adler32 bytes
    s.out = out_data;
    s.out_cap = out_capacity;

    int bfinal;
    do {
        bfinal = inflate_block(&s);
        if (bfinal < 0) return -1;
    } while (!bfinal);

    *out_size = s.out_pos;
    return 0;
}

// ============================================================================
// Scanline Unfiltering
// ============================================================================
static inline int abs_val(int x) { return x < 0 ? -x : x; }

static uint8_t paeth_predictor(uint8_t a, uint8_t b, uint8_t c) {
    int p = a + b - c;
    int pa = abs_val(p - a);
    int pb = abs_val(p - b);
    int pc = abs_val(p - c);
    if (pa <= pb && pa <= pc) return a;
    if (pb <= pc) return b;
    return c;
}

static void unfilter_scanline(uint8_t filter_type, uint8_t* line, const uint8_t* prev_line, int bpp, int stride) {
    for (int i = 0; i < stride; i++) {
        uint8_t raw = line[i];
        uint8_t a = (i >= bpp) ? line[i - bpp] : 0;
        uint8_t b = prev_line ? prev_line[i] : 0;
        uint8_t c = (prev_line && i >= bpp) ? prev_line[i - bpp] : 0;

        switch (filter_type) {
            case 0: break; // None
            case 1: line[i] = raw + a; break; // Sub
            case 2: line[i] = raw + b; break; // Up
            case 3: line[i] = raw + (uint8_t)((a + b) / 2); break; // Average
            case 4: line[i] = raw + paeth_predictor(a, b, c); break; // Paeth
            default: break;
        }
    }
}

static inline uint32_t read_u32(const uint8_t* p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}

} // anonymous namespace

// ============================================================================
// PngDecoder Implementation
// ============================================================================
Result PngDecoder::decode(const uint8_t* buffer, size_t size, Bitmap& out_bitmap) {
    if (!buffer || size < 33) {
        return Result::Fail(ErrorCode::InvalidParameter);
    }

    // Verify PNG magic signature: \x89PNG\r\n\x1a\n
    if (buffer[0] != 0x89 || buffer[1] != 0x50 || buffer[2] != 0x4E || buffer[3] != 0x47 ||
        buffer[4] != 0x0D || buffer[5] != 0x0A || buffer[6] != 0x1A || buffer[7] != 0x0A) {
        return Result::Fail(ErrorCode::InvalidParameter);
    }

    uint32_t w = 0, h = 0;
    uint8_t bit_depth = 0;
    uint8_t color_type = 0;
    uint8_t interlace = 0;

    // Buffer for concatenating IDAT chunks
    uint32_t idat_cap = (uint32_t)size;
    uint8_t* idat_data = (uint8_t*)malloc(idat_cap);
    if (!idat_data) {
        return Result::Fail(ErrorCode::OutOfMemory);
    }
    uint32_t idat_size = 0;

    uint8_t palette[768] = {0};
    bool has_palette = false;

    size_t offset = 8;
    while (offset + 8 <= size) {
        uint32_t chunk_length = read_u32(buffer + offset);
        const uint8_t* chunk_type = buffer + offset + 4;
        const uint8_t* chunk_data = buffer + offset + 8;

        if (offset + 12 + chunk_length > size) break;

        if (chunk_type[0] == 'I' && chunk_type[1] == 'H' && chunk_type[2] == 'D' && chunk_type[3] == 'R') {
            w = read_u32(chunk_data);
            h = read_u32(chunk_data + 4);
            bit_depth = chunk_data[8];
            color_type = chunk_data[9];
            interlace = chunk_data[12];
        } else if (chunk_type[0] == 'P' && chunk_type[1] == 'L' && chunk_type[2] == 'T' && chunk_type[3] == 'E') {
            uint32_t pal_len = chunk_length < 768 ? chunk_length : 768;
            memcpy(palette, chunk_data, pal_len);
            has_palette = true;
        } else if (chunk_type[0] == 'I' && chunk_type[1] == 'D' && chunk_type[2] == 'A' && chunk_type[3] == 'T') {
            if (idat_size + chunk_length > idat_cap) {
                uint32_t new_cap = (idat_cap * 2) + chunk_length;
                uint8_t* new_buf = (uint8_t*)realloc(idat_data, new_cap);
                if (new_buf) {
                    idat_data = new_buf;
                    idat_cap = new_cap;
                }
            }
            if (idat_size + chunk_length <= idat_cap) {
                memcpy(idat_data + idat_size, chunk_data, chunk_length);
                idat_size += chunk_length;
            }
        } else if (chunk_type[0] == 'I' && chunk_type[1] == 'E' && chunk_type[2] == 'N' && chunk_type[3] == 'D') {
            break;
        }

        offset += 12 + chunk_length;
    }

    if (w == 0 || h == 0 || idat_size == 0 || bit_depth != 8 || interlace != 0) {
        free(idat_data);
        return Result::Fail(ErrorCode::InvalidParameter);
    }

    int bytes_per_pixel = 1;
    if (color_type == 2) bytes_per_pixel = 3;       // RGB
    else if (color_type == 4) bytes_per_pixel = 2;  // Grayscale + Alpha
    else if (color_type == 6) bytes_per_pixel = 4;  // RGBA

    uint32_t stride_raw = w * bytes_per_pixel;
    uint32_t uncompressed_size = (stride_raw + 1) * h;
    uint8_t* uncompressed = (uint8_t*)malloc(uncompressed_size);
    if (!uncompressed) {
        free(idat_data);
        return Result::Fail(ErrorCode::OutOfMemory);
    }

    uint32_t decomp_size = 0;
    int res = inflate_decompress(idat_data, idat_size, uncompressed, uncompressed_size, &decomp_size);
    free(idat_data);

    if (res < 0) {
        free(uncompressed);
        return Result::Fail(ErrorCode::InvalidState);
    }

    // Allocate 32-bpp ARGB target buffer
    uint32_t* argb_pixels = (uint32_t*)malloc(w * h * sizeof(uint32_t));
    if (!argb_pixels) {
        free(uncompressed);
        return Result::Fail(ErrorCode::OutOfMemory);
    }

    const uint8_t* prev_line = nullptr;
    for (uint32_t y = 0; y < h; y++) {
        uint8_t* line_start = uncompressed + y * (stride_raw + 1);
        uint8_t filter = line_start[0];
        uint8_t* line_data = line_start + 1;

        unfilter_scanline(filter, line_data, prev_line, bytes_per_pixel, stride_raw);

        for (uint32_t x = 0; x < w; x++) {
            uint32_t pixel = 0xFF000000;
            if (color_type == 6) { // RGBA
                uint8_t r = line_data[x * 4];
                uint8_t g = line_data[x * 4 + 1];
                uint8_t b = line_data[x * 4 + 2];
                uint8_t a = line_data[x * 4 + 3];
                pixel = ((uint32_t)a << 24) | ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
            } else if (color_type == 2) { // RGB
                uint8_t r = line_data[x * 3];
                uint8_t g = line_data[x * 3 + 1];
                uint8_t b = line_data[x * 3 + 2];
                pixel = 0xFF000000 | ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
            } else if (color_type == 3 && has_palette) { // Indexed
                uint8_t idx = line_data[x];
                uint8_t r = palette[idx * 3];
                uint8_t g = palette[idx * 3 + 1];
                uint8_t b = palette[idx * 3 + 2];
                pixel = 0xFF000000 | ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
            } else if (color_type == 0) { // Grayscale
                uint8_t g = line_data[x];
                pixel = 0xFF000000 | ((uint32_t)g << 16) | ((uint32_t)g << 8) | (uint32_t)g;
            } else if (color_type == 4) { // Grayscale + Alpha
                uint8_t g = line_data[x * 2];
                uint8_t a = line_data[x * 2 + 1];
                pixel = ((uint32_t)a << 24) | ((uint32_t)g << 16) | ((uint32_t)g << 8) | (uint32_t)g;
            }
            argb_pixels[y * w + x] = pixel;
        }
        prev_line = line_data;
    }

    free(uncompressed);

    // Transfer ownership to caller's Bitmap
    out_bitmap = Bitmap(w, h, argb_pixels, true);
    return Result::Ok();
}

Result PngDecoder::decode_file(const char* path, Bitmap& out_bitmap) {
    if (!path) return Result::Fail(ErrorCode::InvalidParameter);

    int fd = open(path, O_RDONLY);
    if (fd < 0) return Result::Fail(ErrorCode::ResourceNotFound);

    int64_t file_len = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);

    if (file_len <= 0 || file_len > 32 * 1024 * 1024) { // 32 MB sanity limit
        close(fd);
        return Result::Fail(ErrorCode::InvalidParameter);
    }

    uint8_t* buf = (uint8_t*)malloc((size_t)file_len);
    if (!buf) {
        close(fd);
        return Result::Fail(ErrorCode::OutOfMemory);
    }

    ssize_t bytes_read = read(fd, buf, (size_t)file_len);
    close(fd);

    if (bytes_read != file_len) {
        free(buf);
        return Result::Fail(ErrorCode::ResourceNotFound);
    }

    Result res = decode(buf, (size_t)file_len, out_bitmap);
    free(buf);
    return res;
}

// ============================================================================
// Image Factory Methods
// ============================================================================
Image Image::from_memory(const uint8_t* data, size_t size) {
    Bitmap bmp;
    if (PngDecoder::decode(data, size, bmp).is_ok()) {
        return Image(static_cast<Bitmap&&>(bmp));
    }
    return Image();
}

Image Image::from_file(const char* path) {
    Bitmap bmp;
    if (PngDecoder::decode_file(path, bmp).is_ok()) {
        return Image(static_cast<Bitmap&&>(bmp));
    }
    return Image();
}

Image Image::create_solid(uint32_t w, uint32_t h, Color color) {
    if (w == 0 || h == 0) return Image();
    uint32_t* px = (uint32_t*)malloc(w * h * sizeof(uint32_t));
    if (!px) return Image();

    uint32_t val = color.argb();
    for (uint32_t i = 0; i < w * h; i++) {
        px[i] = val;
    }
    return Image(Bitmap(w, h, px, true));
}

} // namespace bos
