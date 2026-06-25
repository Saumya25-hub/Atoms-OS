#include "command.h"
#include "../libbos/include/bos.h"

static void cmd_help(int argc, char** argv) {
    (void)argc;
    (void)argv;
    command_print_help();
}

static void cmd_ver(int argc, char** argv) {
    (void)argc;
    (void)argv;
    bos_print("Signatures OS v1.0 - Shell V3\n");
}

static void cmd_about(int argc, char** argv) {
    (void)argc;
    (void)argv;
    bos_print("\n====================================================\n");
    bos_print("               SIGNATURES OS - V1                   \n");
    bos_print("====================================================\n\n");
    bos_print("           Architect & Creator: SAUMYA              \n\n");
    bos_print("       \"Not just an OS. A digital legacy.\"        \n");
    bos_print("     A Masterpiece of System Design and Passion.    \n\n");
    bos_print("====================================================\n\n");
}

static void cmd_cls(int argc, char** argv) {
    (void)argc;
    (void)argv;
    for (int i = 0; i < 25; i++) {
        bos_print("\n");
    }
}

static void cmd_echo(int argc, char** argv) {
    for (int i = 1; i < argc; i++) {
        bos_print(argv[i]);
        if (i < argc - 1) bos_print(" ");
    }
    bos_print("\n");
}

static void cmd_time(int argc, char** argv) {
    (void)argc;
    (void)argv;
    bos_print("Time command not yet implemented.\n");
}

static void cmd_date(int argc, char** argv) {
    (void)argc;
    (void)argv;
    bos_print("Date command not yet implemented.\n");
}

static void cmd_test(int argc, char** argv) {
    (void)argc;
    (void)argv;
    bos_print("Starting Validation & Stress Test Framework...\n");
    // Spawn tests.elf
    uint64_t pid = bos_spawn("tests.elf");
    if (pid == 0) {
        bos_print("ERROR: Could not spawn tests.elf. (Maybe it was excluded from this build?)\n");
    } else {
        // Wait for it (we just yield loop for a bit, or assume OS handles interactive spawn correctly)
        // Since the shell doesn't block properly yet, it will just run concurrently.
        // That's fine for testing.
    }
}

void commands_sys_init(void) {
    command_register("help", cmd_help, "Show this help message", "System");
    command_register("ver", cmd_ver, "Show OS version", "System");
    command_register("about", cmd_about, "About Signatures OS", "System");
    command_register("cls", cmd_cls, "Clear the screen", "System");
    command_register("echo", cmd_echo, "Print text to screen", "System");
    command_register("time", cmd_time, "Show current time", "System");
    command_register("date", cmd_date, "Show current date", "System");
    command_register("test", cmd_test, "Run Validation & Stress Test Framework", "System");
}

