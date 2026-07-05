#include <stdint.h>
#include <stddef.h>

#define MAX_BITS 16
#define MAX_LIT 288
#define MAX_DIST 32
#define MAX_CODE 19

typedef struct {
    uint16_t counts[MAX_BITS];
    uint16_t symbols[MAX_LIT];
    int max_sym;
} huffman_t;

typedef struct {
    const uint8_t *in;
    uint32_t in_size;
    uint32_t in_pos;
    uint32_t bit_buf;
    int bit_cnt;
    
    uint8_t *out;
    uint32_t out_cap;
    uint32_t out_pos;
} inflate_state_t;

static int read_bits(inflate_state_t *s, int n) {
    while (s->bit_cnt < n) {
        if (s->in_pos >= s->in_size) return -1;
        s->bit_buf |= (s->in[s->in_pos++] << s->bit_cnt);
        s->bit_cnt += 8;
    }
    int res = s->bit_buf & ((1 << n) - 1);
    s->bit_buf >>= n;
    s->bit_cnt -= n;
    return res;
}

static int build_tree(huffman_t *t, const uint8_t *lengths, int num) {
    uint16_t offs[MAX_BITS];
    for (int i = 0; i < MAX_BITS; i++) t->counts[i] = 0;
    for (int i = 0; i < num; i++) t->counts[lengths[i]]++;
    t->counts[0] = 0;
    
    offs[1] = 0;
    for (int i = 1; i < MAX_BITS - 1; i++) offs[i + 1] = offs[i] + t->counts[i];
    
    for (int i = 0; i < num; i++) {
        if (lengths[i]) t->symbols[offs[lengths[i]]++] = i;
    }
    t->max_sym = num;
    return 0;
}

static int decode_symbol(inflate_state_t *s, huffman_t *t) {
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

static int inflate_uncompressed(inflate_state_t *s) {
    s->bit_buf = 0; s->bit_cnt = 0;
    if (s->in_pos + 4 > s->in_size) return -1;
    uint16_t len = s->in[s->in_pos] | (s->in[s->in_pos+1]<<8);
    uint16_t nlen = s->in[s->in_pos+2] | (s->in[s->in_pos+3]<<8);
    s->in_pos += 4;
    if (len != (uint16_t)~nlen) return -1;
    if (s->in_pos + len > s->in_size || s->out_pos + len > s->out_cap) return -1;
    for (int i=0; i<len; i++) s->out[s->out_pos++] = s->in[s->in_pos++];
    return 0;
}

static const uint16_t length_base[29] = {
    3,4,5,6,7,8,9,10,11,13,15,17,19,23,27,31,35,43,51,59,67,83,99,115,131,163,195,227,258
};
static const uint8_t length_extra[29] = {
    0,0,0,0,0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,0
};
static const uint16_t dist_base[30] = {
    1,2,3,4,5,7,9,13,17,25,33,49,65,97,129,193,257,385,513,769,1025,1537,2049,3073,4097,6145,8193,12289,16385,24577
};
static const uint8_t dist_extra[30] = {
    0,0,0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,10,10,11,11,12,12,13,13
};

static int inflate_huffman(inflate_state_t *s, huffman_t *lt, huffman_t *dt) {
    while (1) {
        int sym = decode_symbol(s, lt);
        if (sym < 0) return -1;
        if (sym < 256) {
            if (s->out_pos >= s->out_cap) return -1;
            s->out[s->out_pos++] = sym;
        } else if (sym == 256) {
            break;
        } else {
            sym -= 257;
            if (sym >= 29) return -1;
            int ext = length_extra[sym];
            int ext_bits = ext ? read_bits(s, ext) : 0;
            if (ext_bits < 0) return -1;
            int len = length_base[sym] + ext_bits;
            
            int dist_sym = decode_symbol(s, dt);
            if (dist_sym < 0 || dist_sym >= 30) return -1;
            ext = dist_extra[dist_sym];
            ext_bits = ext ? read_bits(s, ext) : 0;
            if (ext_bits < 0) return -1;
            int dist = dist_base[dist_sym] + ext_bits;
            
            if (s->out_pos + len > s->out_cap || dist > s->out_pos) return -1;
            for (int i=0; i<len; i++) {
                s->out[s->out_pos] = s->out[s->out_pos - dist];
                s->out_pos++;
            }
        }
    }
    return 0;
}

static int inflate_block(inflate_state_t *s) {
    int bfinal = read_bits(s, 1);
    int btype = read_bits(s, 2);
    if (bfinal < 0 || btype < 0) return -1;
    
    if (btype == 0) {
        if (inflate_uncompressed(s) < 0) return -1;
    } else if (btype == 1 || btype == 2) {
        huffman_t lt, dt;
        if (btype == 1) {
            uint8_t lens[288];
            for (int i=0; i<144; i++) lens[i] = 8;
            for (int i=144; i<256; i++) lens[i] = 9;
            for (int i=256; i<280; i++) lens[i] = 7;
            for (int i=280; i<288; i++) lens[i] = 8;
            build_tree(&lt, lens, 288);
            
            uint8_t dlens[32];
            for (int i=0; i<32; i++) dlens[i] = 5;
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
            
            static const uint8_t clen_order[19] = {16,17,18,0,8,7,9,6,10,5,11,4,12,3,13,2,14,1,15};
            uint8_t clens[19] = {0};
            for (int i=0; i<hclen; i++) {
                int cb = read_bits(s, 3);
                if (cb < 0) return -1;
                clens[clen_order[i]] = cb;
            }
            
            huffman_t ct;
            build_tree(&ct, clens, 19);
            
            uint8_t lens[MAX_LIT + MAX_DIST] = {0};
            int num = hlit + hdist;
            if (num > MAX_LIT + MAX_DIST) return -1;
            
            int i = 0;
            while (i < num) {
                int sym = decode_symbol(s, &ct);
                if (sym < 0) return -1;
                if (sym < 16) {
                    lens[i++] = sym;
                } else {
                    int rep = 0, val = 0;
                    if (sym == 16) {
                        int r = read_bits(s, 2);
                        if (r < 0 || i == 0) return -1;
                        rep = r + 3;
                        val = lens[i-1];
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

int inflate_decompress(const uint8_t* in_data, uint32_t in_size, uint8_t* out_data, uint32_t out_capacity, uint32_t* out_size) {
    if (!in_data || !out_data || !out_size) return -1;
    if (in_size < 6) return -1;
    
    // Check zlib header
    uint8_t cmf = in_data[0];
    if ((cmf & 0x0F) != 8) return -1;
    
    inflate_state_t s = {0};
    s.in = in_data + 2;
    s.in_size = in_size - 6; // ignore zlib header + adler32
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
