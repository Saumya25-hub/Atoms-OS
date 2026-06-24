#ifndef SHELL_COMMAND_H
#define SHELL_COMMAND_H

#include <stdint.h>
#include <stddef.h>

#define MAX_ARGS 16

typedef void (*CommandFunc)(int argc, char** argv);

typedef struct {
    const char* name;
    CommandFunc func;
    const char* description;
    const char* family; // e.g., "System", "Navigation", "Folder", "File", "Debug"
} Command;

// Command Registry
void command_init(void);
void command_register(const char* name, CommandFunc func, const char* desc, const char* family);
void command_execute(char* input_line);
void command_print_help(void);

// BO-DiskHUB Integration
void commands_bodh_init(void);
const char* commands_bodh_get_cwd(void);

// Utils
int command_strcmp(const char *s1, const char *s2);
int command_strncmp(const char *s1, const char *s2, size_t n);

// Modular Command Inits
void commands_sys_init(void);
void commands_debug_init(void);
void commands_bodh_init(void);
void commands_edit_init(void);

#endif // SHELL_COMMAND_H
