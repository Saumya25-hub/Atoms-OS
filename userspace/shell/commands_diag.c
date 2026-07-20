#include "commands_diag.h"
#include "../libbos/include/bos.h"
#include "../../sds/Include/sds.h"
#include "command.h"

// Very simple string compare for the shell
static int strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char *)s1 - *(unsigned char *)s2;
}

// SDS Output Provider for the BOS Shell
static void shell_output_writer(const char* formatted_string) {
    shell_print(formatted_string);
}

// Platform hooks for userspace testing (simulated for now)
uint32_t _SDS_Platform_GetTimestamp(void) { return 0; }
uint32_t _SDS_Platform_GetThreadID(void) { return 1; }
uint32_t _SDS_Platform_GetCPUCore(void) { return 0; }

static bool sds_initialized = false;

void bos_command_diag(int argc, char **argv) {
    if (!sds_initialized) {
        SDS_Init();
        SDS_OutputProvider provider = {
            .provider_name = "BOS Shell",
            .write_string = shell_output_writer,
            .write_raw = NULL
        };
        SDS_RegisterOutputProvider(provider);
        SDS_SetOutputLevel(SDS_LEVEL_DEVELOPER); // Default to Developer for now
        sds_initialized = true;
    }

    if (argc < 2) {
        shell_print("Usage: diag <command>\n");
        shell_print("Commands:\n");
        shell_print("  test    - Run SDS self test\n");
        shell_print("  info    - Show SDS configuration info\n");
        shell_print("  fatal   - Trigger artificial fatal error\n");
        shell_print("  warning - Trigger artificial warning\n");
        shell_print("  level   - Set output level (0-4)\n");
        return;
    }

    if (strcmp(argv[1], "test") == 0) {
        shell_print("Running SDS Self Test...\n");
        SDS_Success(SDS_ENGINE_APP, "TEST-0000", "SDS Self Test Passed.");
    } 
    else if (strcmp(argv[1], "info") == 0) {
        shell_print("SDS Diagnostics System V1\n");
        shell_print("Output Level: Developer\n");
        shell_print("Providers   : BOS Shell\n");
    } 
    else if (strcmp(argv[1], "fatal") == 0) {
        SDS_Fatal(SDS_ENGINE_BV, "BV-GR-0001", "Artificial fatal error triggered from shell.");
    } 
    else if (strcmp(argv[1], "warning") == 0) {
        SDS_Warning(SDS_ENGINE_RK, "RK-NAV-0003", "Artificial warning triggered from shell.");
    }
    else if (strcmp(argv[1], "level") == 0) {
        if (argc >= 3) {
            int lvl = argv[2][0] - '0';
            if (lvl >= 0 && lvl <= 4) {
                SDS_SetOutputLevel((SDS_OutputLevel)lvl);
                shell_print("SDS Output Level updated.\n");
            } else {
                shell_print("Invalid level. Use 0-4.\n");
            }
        } else {
            shell_print("Usage: diag level <0-4>\n");
        }
    }
    else {
        shell_print("Unknown diag subcommand.\n");
    }
}

void commands_diag_init(void) {
    command_register("diag", bos_command_diag, "System Diagnostics & Telemetry", "Debug");
}
