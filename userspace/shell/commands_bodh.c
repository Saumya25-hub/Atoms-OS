#include "command.h"
#include "../libbos/include/bos.h"
#include "../libbos/include/bodiskhub.h"
#include "../atoms/include/atoms.h"
#include "../atoms/include/atom_compiler.h"
#include "../atoms/internal/atom_memory.h"

static char current_path[256] = "/";

static void u_strcpy(char* dest, const char* src) {
    while ((*dest++ = *src++));
}

static size_t u_strlen(const char* s) {
    size_t len = 0;
    while (*s++) len++;
    return len;
}

const char* commands_bodh_get_cwd(void) {
    return current_path;
}

static void print_dec(uint32_t num) {
    if (num == 0) { shell_print("0"); return; }
    char buf[16]; int i = 14; buf[15] = '\0';
    while (num > 0) { buf[i--] = (num % 10) + '0'; num /= 10; }
    shell_print(&buf[i + 1]);
}

static void resolve_absolute_path(const char* input, char* out) {
    if (!input || !out) return;
    if (input[0] == '/') {
        u_strcpy(out, input);
    } else {
        u_strcpy(out, current_path);
        size_t len = u_strlen(out);
        if (len > 0 && out[len - 1] != '/') {
            out[len] = '/';
            out[len + 1] = '\0';
        }
        u_strcpy(out + u_strlen(out), input);
    }
}

static void cmd_pwd(int argc, char** argv) {
    (void)argc; (void)argv;
    shell_print(current_path); shell_print("\n");
}

static void cmd_open(int argc, char** argv) {
    if (argc < 2) { shell_print("Usage: open <directory>\n"); return; }
    resolve_absolute_path(argv[1], current_path);
}

static void cmd_back(int argc, char** argv) {
    (void)argc; (void)argv;
    size_t len = u_strlen(current_path);
    if (len <= 1) return;
    if (current_path[len - 1] == '/') current_path[--len] = '\0';
    while (len > 0 && current_path[len - 1] != '/') len--;
    if (len == 0) len = 1;
    current_path[len] = '\0';
}

static void cmd_mkdir(int argc, char** argv) {
    if (argc < 2) { shell_print("Usage: mkdir <foldername>\n"); return; }
    char target[256];
    resolve_absolute_path(argv[1], target);
    int res = bos_mkdir(target);
    if (res == 0) {
        shell_print("Created directory: "); shell_print(target); shell_print("\n");
    } else {
        shell_print("Failed to create directory.\n");
    }
}

static void cmd_ls(int argc, char** argv) {
    (void)argc; (void)argv;
    shell_print("Listing directory: "); shell_print(current_path); shell_print("\n");
}

void commands_bodh_init(void) {
    command_register("pwd", cmd_pwd, "Print working directory", "FS");
    command_register("open", cmd_open, "Open directory", "FS");
    command_register("back", cmd_back, "Go up one directory", "FS");
    command_register("mkdir", cmd_mkdir, "Create directory", "FS");
    command_register("ls", cmd_ls, "List files", "FS");
}
