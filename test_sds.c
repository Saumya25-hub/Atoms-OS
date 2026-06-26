#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include "sds/Include/sds.h"

// Implement OS hook for timestamp
uint32_t _SDS_Platform_GetTimestamp(void) {
    return (uint32_t)(clock() * 1000 / CLOCKS_PER_SEC);
}

// Implement OS hook for Thread ID
uint32_t _SDS_Platform_GetThreadID(void) {
    return 1; // Fake Thread ID for test
}

// Implement OS hook for CPU Core
uint32_t _SDS_Platform_GetCPUCore(void) {
    return 0; // Fake CPU for test
}

// Output Provider Callback
void MyConsoleWriter(const char* formatted_string) {
    printf("%s", formatted_string);
}

int main(void) {
    SDS_Init();

    SDS_OutputProvider console_provider = {
        .provider_name = "Windows Console",
        .write_string = MyConsoleWriter,
        .write_raw = NULL
    };
    SDS_RegisterOutputProvider(console_provider);

    printf("\n--- LEVEL 1 (SHORT / RELEASE) ---\n");
    SDS_SetOutputLevel(SDS_LEVEL_SHORT);
    SDS_Fatal(SDS_ENGINE_BV, "BV-GR-0001", "");

    printf("\n--- LEVEL 2 (NORMAL) ---\n");
    SDS_SetOutputLevel(SDS_LEVEL_NORMAL);
    SDS_Fatal(SDS_ENGINE_BV, "BV-GR-0001", "");

    printf("\n--- LEVEL 4 (DEVELOPER) ---\n");
    SDS_SetOutputLevel(SDS_LEVEL_DEVELOPER);
    SDS_Fatal(SDS_ENGINE_BV, "BV-GR-0001", "This is an explicit fatal context message.");

    return 0;
}
