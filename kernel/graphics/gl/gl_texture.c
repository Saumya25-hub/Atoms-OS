#include "kernel/graphics/gl/gl_texture.h"
#include "kernel/core/memory/heap/include/heap.h"

void gl_texture_init_object(GLTextureObject* tex, GLuint id) {
    if (!tex) return;

    tex->id         = id;
    tex->wrap_s     = GL_REPEAT;
    tex->wrap_t     = GL_REPEAT;
    tex->min_filter = GL_NEAREST;
    tex->mag_filter = GL_NEAREST;
    tex->defined    = false;
    tex->allocated  = false;
    tex->generation = 1;

    for (int i = 0; i < GL_MAX_MIP_LEVELS; i++) {
        tex->levels[i].defined         = false;
        tex->levels[i].width           = 0;
        tex->levels[i].height          = 0;
        tex->levels[i].internal_format = GL_RGBA;
        tex->levels[i].source_format   = GL_RGBA;
        tex->levels[i].source_type     = GL_UNSIGNED_BYTE;
        tex->levels[i].pixel_data      = NULL;
        tex->levels[i].data_size       = 0;
    }
}

void gl_texture_free_object(GLTextureObject* tex) {
    if (!tex) return;

    for (int i = 0; i < GL_MAX_MIP_LEVELS; i++) {
        if (tex->levels[i].pixel_data) {
            kfree_aligned(tex->levels[i].pixel_data);
            tex->levels[i].pixel_data = NULL;
        }
        tex->levels[i].defined   = false;
        tex->levels[i].width     = 0;
        tex->levels[i].height    = 0;
        tex->levels[i].data_size = 0;
    }

    tex->defined   = false;
    tex->allocated = false;
    tex->generation++;
}

bool gl_texture_upload_image(GLTextureObject* tex, GLint level, GLint internalformat, GLsizei w, GLsizei h, GLint border, GLenum format, GLenum type, const void* pixels, GLint unpack_alignment) {
    if (!tex || level < 0 || level >= GL_MAX_MIP_LEVELS || border != 0) return false;
    if (w <= 0 || h <= 0 || w > GL_MAX_TEXTURE_SIZE || h > GL_MAX_TEXTURE_SIZE) return false;
    if (format != GL_RGB && format != GL_RGBA) return false;
    if (type != GL_UNSIGNED_BYTE) return false;

    if (unpack_alignment != 1 && unpack_alignment != 2 && unpack_alignment != 4 && unpack_alignment != 8) {
        unpack_alignment = 4;
    }

    // Overflow check for allocation size
    uint64_t total_pixels = (uint64_t)w * (uint64_t)h;
    if (total_pixels > 0x0FFFFFFF) return false;

    size_t new_size = (size_t)total_pixels * 4; // Canonical RGBA8
    uint8_t* new_buf = (uint8_t*)kmalloc_aligned(new_size, 16);
    if (!new_buf) return false;

    const uint8_t* src = (const uint8_t*)pixels;

    if (src) {
        size_t bpp = (format == GL_RGBA) ? 4 : 3;
        size_t unaligned_row = (size_t)w * bpp;
        size_t align = (size_t)unpack_alignment;
        size_t row_stride = (unaligned_row + align - 1) & ~(align - 1);

        for (GLsizei r = 0; r < h; r++) {
            const uint8_t* src_row = src + (size_t)r * row_stride;
            uint8_t* dst_row = new_buf + (size_t)r * (size_t)w * 4;

            if (format == GL_RGBA) {
                for (GLsizei c = 0; c < w; c++) {
                    dst_row[c * 4 + 0] = src_row[c * 4 + 2]; // Blue
                    dst_row[c * 4 + 1] = src_row[c * 4 + 1]; // Green
                    dst_row[c * 4 + 2] = src_row[c * 4 + 0]; // Red
                    dst_row[c * 4 + 3] = src_row[c * 4 + 3]; // Alpha
                }
            } else { // GL_RGB
                for (GLsizei c = 0; c < w; c++) {
                    dst_row[c * 4 + 0] = src_row[c * 3 + 2]; // Blue
                    dst_row[c * 4 + 1] = src_row[c * 3 + 1]; // Green
                    dst_row[c * 4 + 2] = src_row[c * 3 + 0]; // Red
                    dst_row[c * 4 + 3] = 255; // Default full alpha
                }
            }
        }
    } else {
        // Zero-fill if NULL pixel buffer supplied
        for (size_t i = 0; i < new_size; i++) new_buf[i] = 0;
    }

    if (tex->levels[level].pixel_data) {
        kfree_aligned(tex->levels[level].pixel_data);
    }

    tex->levels[level].pixel_data      = new_buf;
    tex->levels[level].data_size       = new_size;
    tex->levels[level].width           = w;
    tex->levels[level].height          = h;
    tex->levels[level].internal_format = (format == GL_RGB) ? GL_RGB : GL_RGBA;
    tex->levels[level].source_format   = format;
    tex->levels[level].source_type     = type;
    tex->levels[level].defined         = true;

    if (level == 0) {
        tex->defined   = true;
        tex->allocated = true;
    }

    tex->generation++;

    return true;
}

bool gl_texture_sub_upload_image(GLTextureObject* tex, GLint level, GLint xoffset, GLint yoffset, GLsizei w, GLsizei h, GLenum format, GLenum type, const void* pixels, GLint unpack_alignment) {
    if (!tex || level < 0 || level >= GL_MAX_MIP_LEVELS) return false;
    if (!tex->levels[level].defined || !tex->levels[level].pixel_data) return false;

    if (xoffset < 0 || yoffset < 0 || w < 0 || h < 0) return false;
    if (xoffset + w > tex->levels[level].width || yoffset + h > tex->levels[level].height) return false;

    if (w == 0 || h == 0 || !pixels) return true; // Legal no-op success

    if (format != GL_RGB && format != GL_RGBA) return false;
    if (type != GL_UNSIGNED_BYTE) return false;

    if (unpack_alignment != 1 && unpack_alignment != 2 && unpack_alignment != 4 && unpack_alignment != 8) {
        unpack_alignment = 4;
    }

    const uint8_t* src = (const uint8_t*)pixels;
    size_t bpp = (format == GL_RGBA) ? 4 : 3;
    size_t unaligned_row = (size_t)w * bpp;
    size_t align = (size_t)unpack_alignment;
    size_t row_stride = (unaligned_row + align - 1) & ~(align - 1);

    GLsizei level_w = tex->levels[level].width;
    uint8_t* dst_base = tex->levels[level].pixel_data;

    for (GLsizei r = 0; r < h; r++) {
        const uint8_t* src_row = src + (size_t)r * row_stride;
        uint8_t* dst_row = dst_base + ((size_t)(yoffset + r) * (size_t)level_w + (size_t)xoffset) * 4;

        if (format == GL_RGBA) {
            for (GLsizei c = 0; c < w; c++) {
                dst_row[c * 4 + 0] = src_row[c * 4 + 2]; // Blue
                dst_row[c * 4 + 1] = src_row[c * 4 + 1]; // Green
                dst_row[c * 4 + 2] = src_row[c * 4 + 0]; // Red
                dst_row[c * 4 + 3] = src_row[c * 4 + 3]; // Alpha
            }
        } else { // GL_RGB
            for (GLsizei c = 0; c < w; c++) {
                dst_row[c * 4 + 0] = src_row[c * 3 + 2]; // Blue
                dst_row[c * 4 + 1] = src_row[c * 3 + 1]; // Green
                dst_row[c * 4 + 2] = src_row[c * 3 + 0]; // Red
                dst_row[c * 4 + 3] = 255;
            }
        }
    }

    tex->generation++;
    return true;
}

bool gl_texture_generate_mipmaps(GLTextureObject* tex) {
    if (!tex || !tex->levels[0].defined || !tex->levels[0].pixel_data) return false;

    GLsizei curr_w = tex->levels[0].width;
    GLsizei curr_h = tex->levels[0].height;

    for (GLint level = 1; level < GL_MAX_MIP_LEVELS; level++) {
        if (curr_w == 1 && curr_h == 1) {
            // Reached 1x1, clear any higher levels if present
            if (tex->levels[level].pixel_data) {
                kfree_aligned(tex->levels[level].pixel_data);
                tex->levels[level].pixel_data = NULL;
            }
            tex->levels[level].defined = false;
            continue;
        }

        GLsizei next_w = curr_w / 2; if (next_w < 1) next_w = 1;
        GLsizei next_h = curr_h / 2; if (next_h < 1) next_h = 1;

        size_t next_size = (size_t)next_w * (size_t)next_h * 4;
        uint8_t* next_buf = (uint8_t*)kmalloc_aligned(next_size, 16);
        if (!next_buf) return false;

        const uint8_t* prev_buf = tex->levels[level - 1].pixel_data;

        for (GLsizei y = 0; y < next_h; y++) {
            for (GLsizei x = 0; x < next_w; x++) {
                GLsizei x0 = x * 2;
                GLsizei x1 = (x0 + 1 < curr_w) ? x0 + 1 : x0;
                GLsizei y0 = y * 2;
                GLsizei y1 = (y0 + 1 < curr_h) ? y0 + 1 : y0;

                const uint8_t* p00 = prev_buf + ((size_t)y0 * (size_t)curr_w + (size_t)x0) * 4;
                const uint8_t* p10 = prev_buf + ((size_t)y0 * (size_t)curr_w + (size_t)x1) * 4;
                const uint8_t* p01 = prev_buf + ((size_t)y1 * (size_t)curr_w + (size_t)x0) * 4;
                const uint8_t* p11 = prev_buf + ((size_t)y1 * (size_t)curr_w + (size_t)x1) * 4;

                uint8_t* out = next_buf + ((size_t)y * (size_t)next_w + (size_t)x) * 4;
                for (int c = 0; c < 4; c++) {
                    out[c] = (uint8_t)(((uint32_t)p00[c] + (uint32_t)p10[c] + (uint32_t)p01[c] + (uint32_t)p11[c]) / 4);
                }
            }
        }

        if (tex->levels[level].pixel_data) {
            kfree_aligned(tex->levels[level].pixel_data);
        }

        tex->levels[level].pixel_data      = next_buf;
        tex->levels[level].data_size       = next_size;
        tex->levels[level].width           = next_w;
        tex->levels[level].height          = next_h;
        tex->levels[level].internal_format = tex->levels[0].internal_format;
        tex->levels[level].source_format   = tex->levels[0].source_format;
        tex->levels[level].source_type     = tex->levels[0].source_type;
        tex->levels[level].defined         = true;

        curr_w = next_w;
        curr_h = next_h;
    }

    tex->generation++;
    return true;
}

bool gl_texture_is_complete(const GLTextureObject* tex) {
    if (!tex || !tex->defined || !tex->levels[0].defined || !tex->levels[0].pixel_data) {
        return false;
    }

    if (tex->min_filter == GL_NEAREST || tex->min_filter == GL_LINEAR) {
        return true; // Non-mipmapped filters only require Level 0
    }

    // Mipmapped filters require complete mip chain down to 1x1
    GLsizei exp_w = tex->levels[0].width;
    GLsizei exp_h = tex->levels[0].height;

    for (GLint level = 0; level < GL_MAX_MIP_LEVELS; level++) {
        if (!tex->levels[level].defined || !tex->levels[level].pixel_data) {
            return false;
        }
        if (tex->levels[level].width != exp_w || tex->levels[level].height != exp_h) {
            return false;
        }

        if (exp_w == 1 && exp_h == 1) {
            return true; // Successfully reached 1x1
        }

        exp_w = exp_w / 2; if (exp_w < 1) exp_w = 1;
        exp_h = exp_h / 2; if (exp_h < 1) exp_h = 1;
    }

    return true;
}

bool gl_texture_set_parameter(GLTextureObject* tex, GLenum pname, GLint param) {
    if (!tex) return false;

    switch (pname) {
        case GL_TEXTURE_WRAP_S:
            if (param != GL_REPEAT && param != GL_CLAMP) return false;
            tex->wrap_s = (GLenum)param;
            break;
        case GL_TEXTURE_WRAP_T:
            if (param != GL_REPEAT && param != GL_CLAMP) return false;
            tex->wrap_t = (GLenum)param;
            break;
        case GL_TEXTURE_MIN_FILTER:
            if (param != GL_NEAREST && param != GL_LINEAR &&
                param != GL_NEAREST_MIPMAP_NEAREST && param != GL_LINEAR_MIPMAP_NEAREST &&
                param != GL_NEAREST_MIPMAP_LINEAR && param != GL_LINEAR_MIPMAP_LINEAR) {
                return false;
            }
            tex->min_filter = (GLenum)param;
            break;
        case GL_TEXTURE_MAG_FILTER:
            if (param != GL_NEAREST && param != GL_LINEAR) return false;
            tex->mag_filter = (GLenum)param;
            break;
        default:
            return false;
    }

    return true;
}
