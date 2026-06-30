#ifndef ROOK_PAGES_H
#define ROOK_PAGES_H

#include <stdint.h>

/*
 * ♜ ROOK ENGINE V1.0 — Permanent Numeric Page IDs
 * Chess Rook Philosophy: Pages are addressed strictly by permanent IDs.
 * Never searched by string. Never looked up in hash maps.
 */

#define ROOK_PAGE_BOOT_SPLASH   0x0000
#define ROOK_PAGE_KERNEL_INIT   0x0001
#define ROOK_PAGE_DRIVER_LOAD   0x0002
#define ROOK_PAGE_LOGIN         0x0003
#define ROOK_PAGE_WELCOME       0x0004
#define ROOK_PAGE_DESKTOP       0x0005
#define ROOK_PAGE_EXPLORER      0x0006
#define ROOK_PAGE_SETTINGS      0x0007
#define ROOK_PAGE_SHUTDOWN      0x0008
#define ROOK_PAGE_RECOVERY      0x0009
#define ROOK_PAGE_PANIC         0x000A

#define ROOK_MAX_PAGES          64

/* Event IDs for Event-Driven Progression */
#define ROOK_EVENT_MEM_READY        100
#define ROOK_EVENT_DRIVERS_READY    101
#define ROOK_EVENT_VFS_MOUNTED      102
#define ROOK_EVENT_SCHED_READY      103
#define ROOK_EVENT_BOOT_COMPLETE    104

#endif /* ROOK_PAGES_H */
