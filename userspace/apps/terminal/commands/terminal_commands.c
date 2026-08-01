#include "../include/terminal_api.h"
#include "kernel/drivers/display/display.h"

// ============================================================
// Terminal.BOSX — Command Runtime & Dispatcher
//
// Built-in Commands:
//   Files:  dir, ls, cd, pwd, mkdir, rmdir, copy, move, del, rename, type
//   Proc:   start, tasklist, taskkill, run
//   System: cls, echo, ver, hostname, whoami, time, date, exit
//   Env:    set, unset, env, path
//   Net:    ping, ipconfig, netstat  (delegates to WS2_32.sll)
//   Diag:   mem, cpu, handles, uptime, diagnostics
//   Shell:  help, history, alias, clearhistory, reload
//
// All file ops delegate to KERNEL32.sll / SHELL32.sll
// All net ops  delegate to WS2_32.sll
// All proc ops delegate to KERNEL32.sll / BOSLL.sll
// ============================================================

// ---- File Commands ----
static int cmd_dir    (int argc, const char* argv[]) { (void)argc;(void)argv; display_print("[CMD:dir] Directory listing via KERNEL32.GetFileList()\n");    return 0; }
static int cmd_ls     (int argc, const char* argv[]) { (void)argc;(void)argv; display_print("[CMD:ls] Alias for dir.\n");                                       return 0; }
static int cmd_cd     (int argc, const char* argv[]) { (void)argc;(void)argv; display_print("[CMD:cd] Change directory via KERNEL32.SetCurrentDirectory()\n");  return 0; }
static int cmd_pwd    (int argc, const char* argv[]) { (void)argc;(void)argv; display_print("[CMD:pwd] Print working directory via KERNEL32.GetCurrentDirectory()\n"); return 0; }
static int cmd_mkdir  (int argc, const char* argv[]) { (void)argc;(void)argv; display_print("[CMD:mkdir] Create directory via KERNEL32.CreateDirectory()\n");   return 0; }
static int cmd_rmdir  (int argc, const char* argv[]) { (void)argc;(void)argv; display_print("[CMD:rmdir] Remove directory via KERNEL32.RemoveDirectory()\n");   return 0; }
static int cmd_copy   (int argc, const char* argv[]) { (void)argc;(void)argv; display_print("[CMD:copy] File copy via SHELL32.FileCopy()\n");                    return 0; }
static int cmd_move   (int argc, const char* argv[]) { (void)argc;(void)argv; display_print("[CMD:move] File move via SHELL32.FileMove()\n");                    return 0; }
static int cmd_del    (int argc, const char* argv[]) { (void)argc;(void)argv; display_print("[CMD:del] Delete file via SHELL32.FileDelete()\n");                 return 0; }
static int cmd_rename (int argc, const char* argv[]) { (void)argc;(void)argv; display_print("[CMD:rename] Rename via SHELL32.FileRename()\n");                  return 0; }
static int cmd_type   (int argc, const char* argv[]) { (void)argc;(void)argv; display_print("[CMD:type] Read file via KERNEL32.ReadFile()\n");                   return 0; }

// ---- Process Commands ----
static int cmd_start    (int argc, const char* argv[]) { (void)argc;(void)argv; display_print("[CMD:start] Launch process via KERNEL32.CreateProcess()\n");      return 0; }
static int cmd_tasklist (int argc, const char* argv[]) { (void)argc;(void)argv; display_print("[CMD:tasklist] List processes via KERNEL32.EnumProcesses()\n");   return 0; }
static int cmd_taskkill (int argc, const char* argv[]) { (void)argc;(void)argv; display_print("[CMD:taskkill] Kill process via KERNEL32.TerminateProcess()\n"); return 0; }
static int cmd_run      (int argc, const char* argv[]) { (void)argc;(void)argv; display_print("[CMD:run] Run BOSX app via SHELL32.ShellExecute()\n");             return 0; }

// ---- System Commands ----
static int cmd_cls      (int argc, const char* argv[]) { (void)argc;(void)argv; TerminalClearScreen(); return 0; }
static int cmd_echo     (int argc, const char* argv[]) {
    for (int i = 1; i < argc; i++) { display_print(argv[i]); display_print(" "); }
    display_print("\n");
    return 0;
}
static int cmd_ver      (int argc, const char* argv[]) { (void)argc;(void)argv; display_print("ATOMS OS / Signatures OS — Kernel v0.9.8\n"); return 0; }
static int cmd_hostname (int argc, const char* argv[]) { (void)argc;(void)argv; display_print("ATOMS-HOST\n"); return 0; }
static int cmd_whoami   (int argc, const char* argv[]) { (void)argc;(void)argv; display_print("admin\n"); return 0; }
static int cmd_time     (int argc, const char* argv[]) { (void)argc;(void)argv; display_print("[CMD:time] Get time via KERNEL32.GetSystemTime()\n"); return 0; }
static int cmd_date     (int argc, const char* argv[]) { (void)argc;(void)argv; display_print("[CMD:date] Get date via KERNEL32.GetSystemTime()\n"); return 0; }
static int cmd_exit     (int argc, const char* argv[]) { (void)argc;(void)argv; display_print("[CMD:exit] Closing terminal session.\n"); return 0; }

// ---- Environment Commands ----
static int cmd_set    (int argc, const char* argv[]) { (void)argc;(void)argv; display_print("[CMD:set] Set env variable via terminal_environment_set()\n");    return 0; }
static int cmd_unset  (int argc, const char* argv[]) { (void)argc;(void)argv; display_print("[CMD:unset] Unset env variable.\n"); return 0; }
static int cmd_env    (int argc, const char* argv[]) { (void)argc;(void)argv; display_print("[CMD:env] List all environment variables.\n"); return 0; }
static int cmd_path   (int argc, const char* argv[]) { (void)argc;(void)argv; display_print("[CMD:path] Show PATH variable.\n"); return 0; }

// ---- Network Commands (delegate to WS2_32.sll) ----
static int cmd_ping     (int argc, const char* argv[]) { (void)argc;(void)argv; display_print("[CMD:ping] ICMP ping via WS2_32.sll socket layer.\n"); return 0; }
static int cmd_ipconfig (int argc, const char* argv[]) { (void)argc;(void)argv; display_print("[CMD:ipconfig] Network interface info via WS2_32.sll.\n"); return 0; }
static int cmd_netstat  (int argc, const char* argv[]) { (void)argc;(void)argv; display_print("[CMD:netstat] Network connections via WS2_32.sll.\n"); return 0; }

// ---- Diagnostic Commands (delegate to BOSLL.sll / KERNEL32.sll) ----
static int cmd_mem         (int argc, const char* argv[]) { (void)argc;(void)argv; display_print("[CMD:mem] Memory stats via BOSLL.QueryMemoryStatus()\n");   return 0; }
static int cmd_cpu         (int argc, const char* argv[]) { (void)argc;(void)argv; display_print("[CMD:cpu] CPU stats via BOSLL.QueryCPUStatus()\n");          return 0; }
static int cmd_handles     (int argc, const char* argv[]) { (void)argc;(void)argv; display_print("[CMD:handles] Handle table via KERNEL32.GetHandleCount()\n"); return 0; }
static int cmd_uptime      (int argc, const char* argv[]) { (void)argc;(void)argv; display_print("[CMD:uptime] System uptime via KERNEL32.GetTickCount()\n");  return 0; }
static int cmd_diagnostics (int argc, const char* argv[]) { (void)argc;(void)argv; TerminalDumpDiagnostics(); return 0; }

// ---- Shell Utility Commands ----
static int cmd_help         (int argc, const char* argv[]) { (void)argc;(void)argv; display_print("[CMD:help] Available commands: dir ls cd pwd mkdir rmdir copy move del rename type start tasklist taskkill run cls echo ver hostname whoami time date exit set unset env path ping ipconfig netstat mem cpu handles uptime diagnostics help history alias clearhistory reload\n"); return 0; }
static int cmd_history      (int argc, const char* argv[]) { (void)argc;(void)argv; display_print("[CMD:history] Command history via terminal_history_get()\n"); return 0; }
static int cmd_alias        (int argc, const char* argv[]) { (void)argc;(void)argv; display_print("[CMD:alias] Register alias via TerminalRegisterAlias()\n"); return 0; }
static int cmd_clearhistory (int argc, const char* argv[]) { (void)argc;(void)argv; display_print("[CMD:clearhistory] History cleared.\n"); return 0; }
static int cmd_reload       (int argc, const char* argv[]) { (void)argc;(void)argv; display_print("[CMD:reload] Reloading terminal environment.\n"); return 0; }

// ---- Dispatch Table ----
typedef struct { const char* name; TERMINAL_COMMAND_HANDLER fn; } CMD_ENTRY;

static const CMD_ENTRY s_dispatch_table[] = {
    { "dir",         cmd_dir         }, { "ls",          cmd_ls          },
    { "cd",          cmd_cd          }, { "pwd",         cmd_pwd         },
    { "mkdir",       cmd_mkdir       }, { "rmdir",       cmd_rmdir       },
    { "copy",        cmd_copy        }, { "move",        cmd_move        },
    { "del",         cmd_del         }, { "rename",      cmd_rename      },
    { "type",        cmd_type        }, { "start",       cmd_start       },
    { "tasklist",    cmd_tasklist    }, { "taskkill",    cmd_taskkill    },
    { "run",         cmd_run         }, { "cls",         cmd_cls         },
    { "echo",        cmd_echo        }, { "ver",         cmd_ver         },
    { "hostname",    cmd_hostname    }, { "whoami",      cmd_whoami      },
    { "time",        cmd_time        }, { "date",        cmd_date        },
    { "exit",        cmd_exit        }, { "set",         cmd_set         },
    { "unset",       cmd_unset       }, { "env",         cmd_env         },
    { "path",        cmd_path        }, { "ping",        cmd_ping        },
    { "ipconfig",    cmd_ipconfig    }, { "netstat",     cmd_netstat     },
    { "mem",         cmd_mem         }, { "cpu",         cmd_cpu         },
    { "handles",     cmd_handles     }, { "uptime",      cmd_uptime      },
    { "diagnostics", cmd_diagnostics }, { "help",        cmd_help        },
    { "history",     cmd_history     }, { "alias",       cmd_alias       },
    { "clearhistory",cmd_clearhistory}, { "reload",      cmd_reload      },
};

#define DISPATCH_TABLE_COUNT ((uint32_t)(sizeof(s_dispatch_table) / sizeof(s_dispatch_table[0])))

// Simple string compare helper
static bool str_eq(const char* a, const char* b) {
    while (*a && *b) { if (*a != *b) return false; a++; b++; }
    return *a == *b;
}

int terminal_commands_dispatch(int argc, const char* argv[]) {
    if (argc == 0 || !argv[0]) return 0;
    for (uint32_t i = 0; i < DISPATCH_TABLE_COUNT; i++) {
        if (str_eq(argv[0], s_dispatch_table[i].name)) {
            return s_dispatch_table[i].fn(argc, argv);
        }
    }
    display_print("[TERMINAL] Unknown command: ");
    display_print(argv[0]);
    display_print(" (type 'help' for available commands)\n");
    return -1;
}
