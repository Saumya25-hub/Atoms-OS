#include "command.h"
#include "../libbos/include/bos.h"

#define MAX_COMMANDS 64

static Command cmd_registry[MAX_COMMANDS];
static int cmd_count = 0;

int command_strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char *)s1 - *(unsigned char *)s2;
}

int command_strncmp(const char *s1, const char *s2, size_t n) {
    while (n && *s1 && (*s1 == *s2)) {
        s1++;
        s2++;
        n--;
    }
    if (n == 0) return 0;
    return (unsigned char)*s1 - (unsigned char)*s2;
}

static ShellOutputSink g_shell_sink = NULL;
static void* g_shell_context = NULL;

void shell_print(const char* str) {
    if (g_shell_sink) {
        g_shell_sink(g_shell_context, str);
    } else {
#ifndef BWE_KERNEL
        extern void bos_print(const char*);
        bos_print(str);
#endif
    }
}

void shell_print_dec(uint64_t num) {
    if (num == 0) {
        shell_print("0");
        return;
    }
    char buf[32];
    int i = 30;
    buf[31] = '\0';
    while (num > 0) {
        buf[i--] = (num % 10) + '0';
        num /= 10;
    }
    shell_print(&buf[i + 1]);
}

void command_init(void) {
    cmd_count = 0;
    commands_sys_init();
#ifndef BWE_KERNEL
    commands_debug_init();
    commands_bodh_init();
    commands_edit_init();
    commands_diag_init();
#endif
}

void command_register(const char* name, CommandFunc func, const char* desc, const char* family) {
    if (cmd_count < MAX_COMMANDS) {
        cmd_registry[cmd_count].name = name;
        cmd_registry[cmd_count].func = func;
        cmd_registry[cmd_count].description = desc;
        cmd_registry[cmd_count].family = family;
        cmd_count++;
    }
}

void command_execute(char* input_line) {
    if (!input_line || input_line[0] == '\0') return;

    char* argv[MAX_ARGS];
    int argc = 0;

    // Very simple tokenizer
    int in_arg = 0;
    for (int i = 0; input_line[i] != '\0'; i++) {
        if (input_line[i] == ' ' || input_line[i] == '\t') {
            input_line[i] = '\0';
            in_arg = 0;
        } else if (!in_arg) {
            if (argc < MAX_ARGS) {
                argv[argc++] = &input_line[i];
            }
            in_arg = 1;
        }
    }

    if (argc == 0) return;

    for (int i = 0; i < cmd_count; i++) {
        if (command_strcmp(cmd_registry[i].name, argv[0]) == 0) {
            cmd_registry[i].func(argc, argv);
            return;
        }
    }

    // If no command found, try to spawn it as an executable
    char exec_path[128];
    int j = 0;
    exec_path[j++] = '/';
    
    int has_ext = 0;
    for (int k = 0; argv[0][k] != '\0' && j < 120; k++) {
        char c = argv[0][k];
        if (c >= 'a' && c <= 'z') c -= 32; // Uppercase
        if (c == '.') has_ext = 1;
        exec_path[j++] = c;
    }
    
    if (!has_ext) {
        exec_path[j++] = '.';
        exec_path[j++] = 'E';
        exec_path[j++] = 'L';
        exec_path[j++] = 'F';
    }
    exec_path[j++] = '\0';

    uint64_t pid = bos_spawn(exec_path);
    if (pid == (uint64_t)-1) {
        shell_print("Unknown command or missing file: ");
        shell_print(argv[0]);
        shell_print("\n");
    } else {
        shell_print("Spawned process ");
        shell_print(exec_path);
        shell_print(" with PID ");
        shell_print_dec(pid);
        shell_print("\n");
    }
}

void command_print_help(void) {
    shell_print("BOS Shell V3 Commands:\n\n");
    
    shell_print("Available Commands:\n");
    for (int i = 0; i < cmd_count; i++) {
        shell_print("  ");
        shell_print(cmd_registry[i].name);
        
        int len = 0;
        const char* c = cmd_registry[i].name;
        while (*c++) len++;
        
        for (int p = len; p < 12; p++) shell_print(" ");
        
        shell_print("- ");
        shell_print(cmd_registry[i].description);
        shell_print(" [");
        shell_print(cmd_registry[i].family);
        shell_print("]\n");
    }
}

void Shell_ExecuteCommand(const char* command, ShellOutputSink sink, void* context) {
    g_shell_sink = sink;
    g_shell_context = context;
    
    // Copy the const command string into a mutable buffer for parsing
    char buf[256];
    int len = 0;
    while (command[len] && len < 255) {
        buf[len] = command[len];
        len++;
    }
    buf[len] = '\0';
    
    command_execute(buf);
    
    g_shell_sink = NULL;
    g_shell_context = NULL;
}
