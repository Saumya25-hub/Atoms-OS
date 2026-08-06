#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

int main(void) {
    FILE* f = fopen("build/frame1.jpg", "rb");
    if (!f) return 1;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t* buf = malloc(sz);
    fread(buf, 1, sz, f);
    fclose(f);

    int p = 0;
    while (p < sz - 4) {
        if (buf[p] == 0xFF && buf[p+1] == 0xC4) {
            int len = (buf[p+2] << 8) | buf[p+3];
            printf("DHT marker at %d, len=%d:\n", p, len);
            int q = p + 4;
            while (q < p + 2 + len) {
                uint8_t info = buf[q++];
                uint8_t type = (info >> 4) & 1;
                uint8_t idx = info & 0xF;
                printf("  Table type=%d (0=DC,1=AC), idx=%d:\n", type, idx);
                uint8_t bits[17];
                int tot = 0;
                for (int i=1; i<=16; i++) {
                    bits[i] = buf[q++];
                    tot += bits[i];
                }
                printf("    Bits[1..16]: ");
                for (int i=1; i<=16; i++) printf("%d ", bits[i]);
                printf("\n    Huffval (%d syms): ", tot);
                for (int i=0; i<tot; i++) printf("%02X ", buf[q++]);
                printf("\n");
            }
            break;
        }
        p++;
    }
    free(buf);
    return 0;
}
