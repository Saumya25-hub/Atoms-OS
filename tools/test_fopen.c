#include <stdio.h>
#include <stdlib.h>

int main(void) {
    FILE* f = fopen("build/frame30.jpg", "rb");
    if (!f) { printf("OPEN FAILED\n"); return 1; }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fclose(f);
    printf("OPEN SUCCESS: sz=%ld\n", sz);
    return 0;
}
