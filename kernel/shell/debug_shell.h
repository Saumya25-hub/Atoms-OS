#ifndef ATOMS_DEBUG_SHELL_H
#define ATOMS_DEBUG_SHELL_H

#include <stdint.h>
#include <stdbool.h>

void debug_shell_init(void);
void debug_shell_run_command(const char *cmd_line);
void debug_shell_thread_entry(void);

#endif // ATOMS_DEBUG_SHELL_H
