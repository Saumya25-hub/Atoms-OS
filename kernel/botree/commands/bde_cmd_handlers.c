#include "../include/botree_cmd.h"
#include "../include/botree_path.h"
#include "../include/botree_nav.h"
#include "../include/botree_namespace.h"
#include "../include/botree_tx.h"
#include "../include/botree.h"
#include "kernel/core/lib/include/string.h"

static BDeCommandSink g_cmd_sink = NULL;
static void* g_cmd_ctx = NULL;

static void cmd_print(const char* str) {
    if (g_cmd_sink) {
        g_cmd_sink(str, g_cmd_ctx);
    }
}

int32_t BDe_ExecuteCommand(const char* command_line, BDeCommandSink sink, void* ctx) {
    if (!command_line || strlen(command_line) == 0) return 0;

    g_cmd_sink = sink;
    g_cmd_ctx = ctx;

    // Tokenize command line
    char buf[256];
    strcpy(buf, command_line);

    char* argv[16];
    int argc = 0;
    int in_arg = 0;

    for (int i = 0; buf[i] != '\0'; i++) {
        if (buf[i] == ' ' || buf[i] == '\t') {
            buf[i] = '\0';
            in_arg = 0;
        } else if (!in_arg) {
            if (argc < 16) argv[argc++] = &buf[i];
            in_arg = 1;
        }
    }

    if (argc == 0) return 0;

    const char* cmd = argv[0];

    // Alias mapping for Windows + Linux compatibility
    if (strcmp(cmd, "ls") == 0 || strcmp(cmd, "dir") == 0 || strcmp(cmd, "vdir") == 0) {
        const char* target = (argc > 1) ? argv[1] : "/";
        BDeDirEntry* entries = NULL;
        uint32_t count = 0;
        if (BDe_ReadDirectory(target, &entries, &count) == 0) {
            cmd_print("Directory Listing:\n");
            for (uint32_t i = 0; i < count; i++) {
                if (entries[i].is_directory) {
                    cmd_print("[DIR]  ");
                } else {
                    cmd_print("[FILE] ");
                }
                cmd_print(entries[i].name);
                cmd_print("\n");
            }
            BDe_FreeDirectoryListing(entries);
        } else {
            cmd_print("Error reading directory.\n");
        }
    } else if (strcmp(cmd, "cp") == 0 || strcmp(cmd, "copy") == 0 || strcmp(cmd, "xcopy") == 0) {
        if (argc < 3) {
            cmd_print("Usage: copy <src> <dest_dir>\n");
        } else {
            BDe_TransactionCopy(argv[1], argv[2]);
            cmd_print("Copy transaction initiated.\n");
        }
    } else if (strcmp(cmd, "mv") == 0 || strcmp(cmd, "move") == 0 || strcmp(cmd, "ren") == 0) {
        if (argc < 3) {
            cmd_print("Usage: move <src> <dest_dir>\n");
        } else {
            BDe_TransactionMove(argv[1], argv[2]);
            cmd_print("Move transaction initiated.\n");
        }
    } else if (strcmp(cmd, "rm") == 0 || strcmp(cmd, "del") == 0 || strcmp(cmd, "erase") == 0 || strcmp(cmd, "unlink") == 0) {
        if (argc < 2) {
            cmd_print("Usage: del <target>\n");
        } else {
            BDe_TransactionDelete(argv[1], false);
            cmd_print("Delete completed.\n");
        }
    } else {
        cmd_print("Unknown command: ");
        cmd_print(cmd);
        cmd_print("\n");
    }

    g_cmd_sink = NULL;
    g_cmd_ctx = NULL;
    return 0;
}
