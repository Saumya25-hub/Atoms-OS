#ifndef ATOMS_SYSCALL_H
#define ATOMS_SYSCALL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define ATOMS_SYSCALL_ABI_VERSION 1U

/* Syscall Return Codes */
#define SYSCALL_OK 0ULL
#define SYSCALL_FAIL ((uint64_t)-1)
#define SYSCALL_INVALID ((uint64_t)-2)
#define SYSCALL_NOT_IMPLEMENTED ((uint64_t)-3)
#define SYSCALL_BAD_ADDRESS ((uint64_t)-4)
#define SYSCALL_TOO_LARGE ((uint64_t)-5)

/* Phase C Mandatory Syscall Numbers */
#define SYS_WRITE 0U
#define SYS_EXIT 1U
#define SYS_GETPID 2U
#define SYS_YIELD 3U
#define SYS_UPTIME 4U
#define SYS_ALLOC 5U
#define SYS_FREE 6U
#define SYS_DEBUG_PRINT 7U

/* ATOMS OS Frozen GUI Syscall Numbers (16 - 23) */
#define SYS_GUI_CREATE_WINDOW       16U
#define SYS_GUI_DESTROY_WINDOW      17U
#define SYS_GUI_SHOW_WINDOW         18U
#define SYS_GUI_SET_BOUNDS          19U
#define SYS_GUI_MAP_SURFACE         20U
#define SYS_GUI_INVALIDATE          21U
#define SYS_GUI_POLL_EVENT          22U
#define SYS_GUI_GET_SCREEN_INFO     23U
#define MAX_SYSCALL                 24U

#define BOS_GUI_EVENT_ABI_VERSION 1U

typedef enum {
    BOS_GUI_EVENT_NONE        = 0,
    BOS_GUI_EVENT_CLICK       = 1,
    BOS_GUI_EVENT_CLOSE       = 2,
    BOS_GUI_EVENT_KEY_DOWN    = 3,
    BOS_GUI_EVENT_KEY_UP      = 4,
    BOS_GUI_EVENT_MOUSE_MOVE  = 5,
    BOS_GUI_EVENT_MOUSE_DOWN  = 6,
    BOS_GUI_EVENT_MOUSE_UP    = 7,
    BOS_GUI_EVENT_FOCUS_GAIN  = 8,
    BOS_GUI_EVENT_FOCUS_LOST  = 9
} BOS_GUIEventType;

typedef struct {
    uint32_t        abi_version; /* Must match BOS_GUI_EVENT_ABI_VERSION */
    uint32_t        type;        /* BOS_GUIEventType */
    uint32_t        window_id;   /* Target Window ID */
    int32_t         mouse_x;     /* Window-local Mouse X */
    int32_t         mouse_y;     /* Window-local Mouse Y */
    uint32_t        mouse_btn;   /* 1=Left, 2=Right, 4=Middle */
    uint32_t        key_code;    /* Hardware Keycode */
    uint32_t        ascii_char;  /* Printable ASCII char */
    uint32_t        modifiers;   /* Shift=1, Ctrl=2, Alt=4 */
    uint32_t        reserved;    /* 64-bit alignment padding */
} BOS_GUIEvent;

/* Usermode Window Bounds for Security Validation */
#define USER_WINDOW_MIN 0x40000000ULL
#define USER_WINDOW_MAX 0x80000000ULL

/* Syscall Frame Layout (ABI-matched with syscall_entry.asm) */
typedef struct ATOMS_SyscallFrame {
  uint64_t user_rsp;    /* 0 */
  uint64_t user_rip;    /* 8 */
  uint64_t user_rflags; /* 16 */
  uint64_t number;      /* 24 */
  uint64_t args[6];     /* 32..79 */
  uint64_t result;      /* 80 */
  uint64_t entry_task;  /* 88 */
  uint32_t pid;         /* 96 */
  uint32_t tid;         /* 100 */
  uint32_t cpu_id;      /* 104 */
  uint16_t nesting;     /* 108 */
  uint8_t terminated;   /* 110 */
  uint8_t return_mode;  /* 111 */
} ATOMS_SyscallFrame;

#define ATOMS_SYSCALL_FRAME_SIZE 112U
#define ATOMS_SYSCALL_RETURN_SYSRET 0U
#define ATOMS_SYSCALL_RETURN_IRET 1U
#define ATOMS_SYSCALL_RETURN_BLOCK 2U

/* MSR Constants */
#define IA32_EFER_MSR 0xC0000080U
#define IA32_STAR_MSR 0xC0000081U
#define IA32_LSTAR_MSR 0xC0000082U
#define IA32_FMASK_MSR 0xC0000084U

/* Function Prototypes */
bool syscall_phase5_self_test(void);

void syscall_init_msrs(void);
void syscall_init(void);

bool syscall_validate_user_ptr(const void *ptr, size_t size);

uint64_t syscall_dispatch(uint64_t id, uint64_t a1, uint64_t a2, uint64_t a3,
                         uint64_t a4, uint64_t a5, uint64_t a6);

uint64_t syscall_handler(ATOMS_SyscallFrame *frame);
uint64_t syscall_prepare_return(ATOMS_SyscallFrame *frame);

/* Legacy/Internal Helper Functions */
void sys_yield(void);
void sys_sleep(uint64_t ticks);
uint64_t sys_uptime(void);
uint64_t sys_getpid(void);

/* Syscall Services */
uint64_t sys_service_write(const char *user_str, size_t len);
uint64_t sys_service_exit(int code);
uint64_t sys_service_getpid(void);
uint64_t sys_service_yield(void);
uint64_t sys_service_uptime(void);
uint64_t sys_service_alloc(size_t size);
uint64_t sys_service_free(void *ptr);
uint64_t sys_service_debug_print(const char *msg);

/* GUI Syscall Services */
uint64_t sys_service_gui_create_window(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t flags, const char *title);
uint64_t sys_service_gui_destroy_window(uint32_t win_id);
uint64_t sys_service_gui_show_window(uint32_t win_id, uint32_t visible);
uint64_t sys_service_gui_set_bounds(uint32_t win_id, int32_t x, int32_t y, int32_t w, int32_t h);
uint64_t sys_service_gui_map_surface(uint32_t win_id, uint64_t *out_user_surface_ptr, uint32_t *out_stride_bytes);
uint64_t sys_service_gui_invalidate(uint32_t win_id, int32_t x, int32_t y, int32_t w, int32_t h);
uint64_t sys_service_gui_poll_event(uint32_t win_id, BOS_GUIEvent *out_user_event, uint32_t event_struct_size);
uint64_t sys_service_gui_get_screen_info(uint32_t *out_w, uint32_t *out_h, uint32_t *out_bpp);

/* Certification Routine */
void launch_phase_c_certification(void);

#endif
