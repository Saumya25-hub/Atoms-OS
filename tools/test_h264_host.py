import os
import subprocess

code = '''#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "h264bsd_decoder.h"

int main() {
    FILE* f = fopen("TEST-VIDEO/test1.mp4", "rb");
    if (!f) { printf("Failed to open mp4\\n"); return 1; }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t* d = malloc(sz);
    fread(d, 1, sz, f);
    fclose(f);

    storage_t* storage = h264bsdAlloc();
    if (!storage) { printf("Failed to alloc storage\\n"); return 1; }
    u32 ret = h264bsdInit(storage, 0);
    printf("h264bsdInit ret: %u\\n", ret);

    // Find avcC
    uint8_t* p = d;
    while (p < d + 2000) {
        if (memcmp(p, "avcC", 4) == 0) break;
        p++;
    }
    if (p >= d + 2000) { printf("avcC not found\\n"); return 1; }
    uint8_t* avcc = p + 4;
    uint32_t sps_len = (avcc[6] << 8) | avcc[7];
    uint8_t* sps = avcc + 8;
    uint32_t pps_pos = 8 + sps_len;
    uint32_t pps_len = (avcc[pps_pos+1] << 8) | avcc[pps_pos+2];
    uint8_t* pps = avcc + pps_pos + 3;

    uint8_t sps_annexb[256];
    sps_annexb[0] = 0; sps_annexb[1] = 0; sps_annexb[2] = 0; sps_annexb[3] = 1;
    memcpy(sps_annexb + 4, sps, sps_len);
    u32 readBytes = 0;
    u32 dret = h264bsdDecode(storage, sps_annexb, 4 + sps_len, 0, &readBytes);
    printf("SPS decode ret: %u\\n", dret);

    uint8_t pps_annexb[256];
    pps_annexb[0] = 0; pps_annexb[1] = 0; pps_annexb[2] = 0; pps_annexb[3] = 1;
    memcpy(pps_annexb + 4, pps, pps_len);
    dret = h264bsdDecode(storage, pps_annexb, 4 + pps_len, 0, &readBytes);
    printf("PPS decode ret: %u\\n", dret);

    printf("Pic width: %u, height: %u\\n", h264bsdPicWidth(storage)*16, h264bsdPicHeight(storage)*16);

    // Feed sample 0 at offset 32280, size 151
    uint8_t sample0[1024];
    memcpy(sample0, d + 32280, 151);
    uint32_t off = 0;
    while (off + 4 < 151) {
        uint32_t nlen = (sample0[off] << 24) | (sample0[off+1] << 16) | (sample0[off+2] << 8) | sample0[off+3];
        sample0[off] = 0; sample0[off+1] = 0; sample0[off+2] = 0; sample0[off+3] = 1;
        off += 4 + nlen;
    }

    uint8_t* cur = sample0;
    uint32_t rem = 151;
    uint32_t picId = 1;
    while (rem > 0) {
        dret = h264bsdDecode(storage, cur, rem, picId, &readBytes);
        printf("Sample0 NAL decode ret: %u, bytes read: %u\\n", dret, readBytes);
        if (dret == 1 /* H264BSD_PIC_RDY */) {
            printf("PICTURE READY!\\n");
        }
        if (readBytes == 0 || readBytes > rem) break;
        cur += readBytes;
        rem -= readBytes;
    }

    u32 outPicId = 0, isIdr = 0, numErr = 0;
    u8* pic = h264bsdNextOutputPicture(storage, &outPicId, &isIdr, &numErr);
    printf("Output picture: %p (picId: %u, isIdr: %u)\\n", (void*)pic, outPicId, isIdr);

    h264bsdShutdown(storage);
    h264bsdFree(storage);
    free(d);
    return 0;
}
'''
with open('build/test_host_decode.c', 'w') as f:
    f.write(code)

cmd = 'clang -Ibuild/h264bsd_repo/src build/test_host_decode.c build/h264bsd_repo/src/*.c -o build/test_host_decode.exe'
res = subprocess.run(cmd, shell=True, capture_output=True, text=True)
if res.returncode != 0:
    print('Compile error:\\n', res.stderr)
else:
    run_res = subprocess.run('build\\\\test_host_decode.exe', shell=True, capture_output=True, text=True)
    print('Run output:\\n' + run_res.stdout)
