#include "../include/terminal_api.h"
#include "kernel/drivers/display/display.h"

// ============================================================
// Terminal.BOSX — Command Parser Engine
// Tokenizes raw input into argc/argv for the dispatcher.
// No kernel access. Pure string processing.
// ============================================================

static char  s_token_buf[TERMINAL_MAX_LINE_LEN];
static const char* s_argv[TERMINAL_MAX_ARGS];
static int   s_argc = 0;

int terminal_parser_parse(const char* line, int* out_argc, const char** out_argv[]) {
    if (!line || !out_argc || !out_argv) return -1;

    s_argc = 0;
    uint32_t bi = 0;
    uint32_t li = 0;

    while (line[li]) {
        // Skip leading spaces
        while (line[li] == ' ' || line[li] == '\t') li++;
        if (!line[li]) break;
        if (s_argc >= TERMINAL_MAX_ARGS) break;

        s_argv[s_argc] = &s_token_buf[bi];
        s_argc++;

        // Copy token until space or end
        while (line[li] && line[li] != ' ' && line[li] != '\t' && bi < TERMINAL_MAX_LINE_LEN - 1) {
            s_token_buf[bi++] = line[li++];
        }
        s_token_buf[bi++] = '\0';
    }

    *out_argc = s_argc;
    *out_argv = s_argv;
    return 0;
}

bool TerminalExecute(const char* command) {
    if (!command || command[0] == '\0') return true;

    display_print("[TERMINAL_PARSER] Parsing command: ");
    display_print(command);
    display_print("\n");

    int argc = 0;
    const char** argv = 0;
    terminal_parser_parse(command, &argc, &argv);

    if (argc == 0) return true;

    // Dispatch to command runtime
    extern int terminal_commands_dispatch(int argc, const char* argv[]);
    int result = terminal_commands_dispatch(argc, argv);
    (void)result;
    return true;
}
