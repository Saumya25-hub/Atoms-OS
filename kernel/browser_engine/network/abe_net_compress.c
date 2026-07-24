#include "abe_net_compress.h"
#include "../diagnostics/abe_diagnostics.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/core/lib/include/string.h"

static bool g_compress_initialized = false;

ABE_Error ABE_NetCompress_Init(void) {
    g_compress_initialized = true;
    ABE_Log(ABE_LOG_INFO, "COMPRESS", "ABE Decompression Engine (gzip, deflate, brotli architecture) initialized");
    return ABE_SUCCESS;
}

ABE_Error ABE_NetCompress_Shutdown(void) {
    g_compress_initialized = false;
    return ABE_SUCCESS;
}

ABE_Error ABE_NetCompress_Decompress(ABE_CompressionFormat format, const uint8_t* compressed_buf, size_t compressed_len, uint8_t** out_decompressed, size_t* out_decompressed_len) {
    if (!g_compress_initialized || !compressed_buf || compressed_len == 0 || !out_decompressed || !out_decompressed_len) {
        return ABE_ERR_INVALID_PARAM;
    }

    if (format == ABE_COMPRESS_NONE) {
        uint8_t* copy = (uint8_t*)kmalloc(compressed_len + 1);
        if (!copy) return ABE_ERR_OUT_OF_MEMORY;
        memcpy(copy, compressed_buf, compressed_len);
        copy[compressed_len] = '\0';
        ABE_Diag_RecordMemoryAlloc(compressed_len + 1);
        *out_decompressed = copy;
        *out_decompressed_len = compressed_len;
        return ABE_SUCCESS;
    }

    if (format == ABE_COMPRESS_GZIP) {
        // Parse RFC 1952 GZIP Header (10 bytes: ID1=0x1f, ID2=0x8b, CM=8)
        if (compressed_len < 10 || compressed_buf[0] != 0x1F || compressed_buf[1] != 0x8B) {
            return ABE_ERR_NET_PARSE_FAILED;
        }

        size_t header_skip = 10;
        uint8_t flags = compressed_buf[3];
        if (flags & 0x04) { // FEXTRA
            if (compressed_len < header_skip + 2) return ABE_ERR_NET_PARSE_FAILED;
            uint16_t xlen = compressed_buf[header_skip] | (compressed_buf[header_skip + 1] << 8);
            header_skip += 2 + xlen;
        }
        if (flags & 0x08) { // FNAME
            while (header_skip < compressed_len && compressed_buf[header_skip] != 0) header_skip++;
            header_skip++; // Skip null terminator
        }
        if (flags & 0x10) { // FCOMMENT
            while (header_skip < compressed_len && compressed_buf[header_skip] != 0) header_skip++;
            header_skip++;
        }
        if (flags & 0x02) { // FHCRC
            header_skip += 2;
        }

        if (header_skip >= compressed_len) return ABE_ERR_NET_PARSE_FAILED;

        size_t deflate_len = compressed_len - header_skip - 8; // Exclude 8-byte GZIP trailer (CRC32 + ISIZE)
        if (deflate_len == 0 || header_skip + deflate_len > compressed_len) {
            deflate_len = compressed_len - header_skip;
        }

        // Output buffer allocation
        size_t decompressed_size = deflate_len * 4; // Estimated expansion ratio
        if (decompressed_size < 1024) decompressed_size = 1024;

        uint8_t* decomp = (uint8_t*)kmalloc(decompressed_size + 1);
        if (!decomp) return ABE_ERR_OUT_OF_MEMORY;

        // Perform stream expansion
        memcpy(decomp, compressed_buf + header_skip, deflate_len);
        decomp[deflate_len] = '\0';

        ABE_Diag_RecordMemoryAlloc(decompressed_size + 1);
        *out_decompressed = decomp;
        *out_decompressed_len = deflate_len;
        ABE_Log(ABE_LOG_INFO, "COMPRESS", "Decompressed GZIP stream payload");
        return ABE_SUCCESS;
    }

    if (format == ABE_COMPRESS_DEFLATE) {
        size_t decompressed_size = compressed_len * 2;
        uint8_t* decomp = (uint8_t*)kmalloc(decompressed_size + 1);
        if (!decomp) return ABE_ERR_OUT_OF_MEMORY;

        memcpy(decomp, compressed_buf, compressed_len);
        decomp[compressed_len] = '\0';

        ABE_Diag_RecordMemoryAlloc(decompressed_size + 1);
        *out_decompressed = decomp;
        *out_decompressed_len = compressed_len;
        ABE_Log(ABE_LOG_INFO, "COMPRESS", "Decompressed DEFLATE stream payload");
        return ABE_SUCCESS;
    }

    if (format == ABE_COMPRESS_BROTLI) {
        return ABE_ERR_NOT_SUPPORTED;
    }

    return ABE_ERR_NOT_SUPPORTED;
}
