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

/* Phase 7 Standard Memory & Sync Syscalls (8 - 15) */
#define SYS_MMAP            8U
#define SYS_MUNMAP          9U
#define SYS_MPROTECT        10U
#define SYS_FUTEX           11U
#define SYS_CLOCK_GETTIME   12U
#define SYS_NANOSLEEP       13U
#define SYS_OPEN            14U
#define SYS_READ            15U

/* ATOMS OS Frozen GUI Syscall Numbers (16 - 24) */
#define SYS_GUI_CREATE_WINDOW       16U
#define SYS_GUI_DESTROY_WINDOW      17U
#define SYS_GUI_SHOW_WINDOW         18U
#define SYS_GUI_SET_BOUNDS          19U
#define SYS_GUI_MAP_SURFACE         20U
#define SYS_GUI_INVALIDATE          21U
#define SYS_GUI_POLL_EVENT          22U
#define SYS_GUI_GET_SCREEN_INFO     23U
#define SYS_GUI_DRAW_WALLPAPER      24U

/* Phase 7 File, Thread & Process Extended Syscalls (25 - 29) */
#define SYS_CLOSE           25U
#define SYS_SEEK            26U
#define SYS_THREAD_SPAWN    27U
#define SYS_THREAD_EXIT     28U
#define SYS_WRITE_FILE      29U

/* Phase 9 Native BOFS / VFS Filesystem Syscalls (30 - 36) */
#define SYS_CREATE          30U
#define SYS_MKDIR           31U
#define SYS_READDIR         32U
#define SYS_UNLINK          33U
#define SYS_RENAME          34U
#define SYS_RMDIR           35U
#define SYS_STAT            36U
#define SYS_EXEC            37U

/* Phase 16-B Chromium Process, IPC, Shared-Memory, & Exception Syscalls (38 - 42) */
#define SYS_WAITPID         38U
#define SYS_IPC_CALL        39U
#define SYS_SHM_CALL        40U
#define SYS_KILL            41U
#define SYS_PROCESS_STATUS  42U
#define SYS_AUDIO_CALL      43U
#define MAX_SYSCALL         44U

/* Syscall Audio Sub-operations */
#define ATOMS_AUDIO_OP_DEVICE_GET_INFO 1U
#define ATOMS_AUDIO_OP_STREAM_CREATE   2U
#define ATOMS_AUDIO_OP_STREAM_DESTROY  3U
#define ATOMS_AUDIO_OP_STREAM_WRITE    4U
#define ATOMS_AUDIO_OP_STREAM_START    5U
#define ATOMS_AUDIO_OP_STREAM_STOP     6U
#define ATOMS_AUDIO_OP_STREAM_PAUSE    7U
#define ATOMS_AUDIO_OP_STREAM_RESUME   8U
#define ATOMS_AUDIO_OP_DEVICE_SET_VOL  9U
#define ATOMS_AUDIO_OP_STREAM_SET_FORMAT 10U
#define ATOMS_AUDIO_OP_STREAM_GET_AVAIL  11U

/* Syscall IPC Sub-operations */
#define ATOMS_IPC_OP_CREATE   1U
#define ATOMS_IPC_OP_CONNECT  2U
#define ATOMS_IPC_OP_SEND     3U
#define ATOMS_IPC_OP_RECV     4U
#define ATOMS_IPC_OP_CLOSE    5U

/* Syscall SHM Sub-operations */
#define ATOMS_SHM_OP_CREATE   1U
#define ATOMS_SHM_OP_OPEN     2U
#define ATOMS_SHM_OP_MAP      3U
#define ATOMS_SHM_OP_UNMAP    4U
#define ATOMS_SHM_OP_DESTROY  5U

typedef struct {
    uint32_t pid;
    uint32_t parent_pid;
    uint32_t state;
    int32_t  exit_code;
    uint32_t thread_count;
    uint64_t cpu_time_ms;
} atoms_process_status_t;

/* Phase 9 ABI Structures */
typedef struct {
    uint64_t st_ino;        /* Inode number */
    uint32_t st_mode;       /* File mode (type + permissions) */
    uint32_t st_nlink;      /* Number of hard links (1 for BOFS) */
    uint32_t st_uid;        /* User ID of owner */
    uint32_t st_gid;        /* Group ID of owner */
    uint64_t st_size;       /* Total size in bytes (64-bit safe) */
    uint64_t st_blocks;     /* Number of 512B blocks allocated */
    uint64_t st_atime;      /* Time of last access */
    uint64_t st_mtime;      /* Time of last modification */
    uint64_t st_ctime;      /* Time of last status change */
} atoms_stat_t;

typedef struct {
    uint64_t d_ino;         /* Inode number */
    uint32_t d_type;        /* File type: 1=REG, 2=DIR */
    uint32_t d_namlen;      /* Length of name in bytes */
    char     d_name[256];   /* Null-terminated name */
} atoms_dirent_t;

/* Memory Protection Flags */
#define PROT_NONE           0x0
#define PROT_READ           0x1
#define PROT_WRITE          0x2
#define PROT_EXEC           0x4

/* Map Flags */
#define MAP_SHARED          0x01
#define MAP_PRIVATE         0x02
#define MAP_FIXED           0x10
#define MAP_ANONYMOUS       0x20
#define MAP_ANON            MAP_ANONYMOUS

/* Futex Operations */
#define FUTEX_WAIT          0
#define FUTEX_WAKE          1
#define FUTEX_REQUEUE       2
#define FUTEX_PRIVATE_FLAG  128


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
bool syscall_validate_user_ptr_writable(const void *ptr, size_t size);
bool syscall_validate_user_string(const char *str, size_t max_len);

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
uint64_t sys_service_gui_draw_wallpaper(uint32_t win_id, int32_t x, int32_t y, int32_t w, int32_t h);

/* Phase 7 Standard Services */

uint64_t sys_service_mmap(uint64_t addr, size_t length, int prot, int flags, int fd, uint64_t offset);
uint64_t sys_service_munmap(uint64_t addr, size_t length);
uint64_t sys_service_mprotect(uint64_t addr, size_t length, int prot);
uint64_t sys_service_futex(uint32_t *uaddr, int op, uint32_t val, const void *timeout);
uint64_t sys_service_clock_gettime(int clock_id, void *tp);
uint64_t sys_service_nanosleep(const void *req, void *rem);
uint64_t sys_service_open(const char *path, int flags, int mode);
uint64_t sys_service_read(int fd, void *buf, size_t count);
uint64_t sys_service_close(int fd);
uint64_t sys_service_seek(int fd, uint64_t offset, int whence);
uint64_t sys_service_thread_spawn(void (*entry)(void*), void *stack_top, void *arg);
uint64_t sys_service_thread_exit(int exit_code);
uint64_t sys_service_write_file(int fd, const void *buf, size_t count);

/* Phase 9 Filesystem Extended Services */
uint64_t sys_service_create(const char *path, int mode);
uint64_t sys_service_mkdir(const char *path, int mode);
uint64_t sys_service_readdir(const char *path, int index, void *out_dirent);
uint64_t sys_service_unlink(const char *path);
uint64_t sys_service_rename(const char *old_path, const char *new_path);
uint64_t sys_service_rmdir(const char *path);
uint64_t sys_service_stat(const char *path, void *out_stat);

/* Phase 10 BOSX Execution Service */
uint64_t sys_service_exec(const char *path, const char **argv, const char **envp);

/* Phase 16-B Chromium Process & IPC Extended Services */
uint64_t sys_service_waitpid(uint32_t pid, int32_t *out_status, uint32_t options);
uint64_t sys_service_ipc_call(uint32_t op, uint64_t a1, uint64_t a2, uint64_t a3);
uint64_t sys_service_shm_call(uint32_t op, uint64_t a1, uint64_t a2, uint64_t a3);
uint64_t sys_service_kill(uint32_t pid, int32_t signal);
uint64_t sys_service_process_status(uint32_t pid, void *out_status_buf);
uint64_t sys_service_audio_call(uint32_t op, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4);

/* Certification Routine */
void launch_phase_c_certification(void);
void launch_phase7_runtime_certification(void);

#endif

