#include "command.h"
#include "../libbos/include/bos.h"

static void cmd_heapinfo(int argc, char** argv) {
    (void)argc;
    (void)argv;
    bos_heapinfo();
}

static void cmd_heapwalk(int argc, char** argv) {
    (void)argc;
    (void)argv;
    bos_heapwalk();
}

static void cmd_heapvalidate(int argc, char** argv) {
    (void)argc;
    (void)argv;
    bos_heapvalidate();
}

static void cmd_dmesg(int argc, char** argv) {
    (void)argc;
    (void)argv;
    bos_dmesg();
}

static void cmd_taskinfo(int argc, char** argv) {
    if (argc < 2) {
        bos_print("Usage: taskinfo <pid>\n");
        return;
    }
    int pid = 0;
    int i = 0;
    while (argv[1][i] >= '0' && argv[1][i] <= '9') {
        pid = pid * 10 + (argv[1][i] - '0');
        i++;
    }
    bos_taskinfo(pid);
}

void commands_debug_init(void) {
    command_register("heapinfo", cmd_heapinfo, "Show kernel heap statistics", "Debug");
    command_register("heapwalk", cmd_heapwalk, "View deep heap block details", "Debug");
    command_register("heapvalidate", cmd_heapvalidate, "Validate heap block integrity", "Debug");
    command_register("dmesg", cmd_dmesg, "View kernel crash/event log", "Debug");
    command_register("taskinfo", cmd_taskinfo, "Show detailed task info", "Debug");
}
