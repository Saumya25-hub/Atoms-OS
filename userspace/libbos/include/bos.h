#ifndef BOS_H
#define BOS_H

#include <stdint.h>
#include <stddef.h>

typedef struct {
    char name[64];
    uint32_t size;
    uint8_t is_directory;
    uint32_t cluster;
} bos_dirent_t;

// Special Key Codes
#define BOS_KEY_UP    0x80
#define BOS_KEY_DOWN  0x81
#define BOS_KEY_LEFT  0x82
#define BOS_KEY_RIGHT 0x83
#define BOS_KEY_ESC   0x84
#define BOS_KEY_F1    0x85
#define BOS_KEY_F2    0x86
#define BOS_KEY_F3    0x87
#define BOS_KEY_F4    0x88
#define BOS_KEY_F5    0x89
#define BOS_KEY_F6    0x8A
#define BOS_KEY_F7    0x8B
#define BOS_KEY_F8    0x8C
#define BOS_KEY_F9    0x8D
#define BOS_KEY_F10   0x8E
#define BOS_KEY_F11   0x8F
#define BOS_KEY_F12   0x90
#define BOS_KEY_HOME  0x91
#define BOS_KEY_END   0x92
#define BOS_KEY_PGUP  0x93
#define BOS_KEY_PGDN  0x94
#define BOS_KEY_INS   0x95
#define BOS_KEY_DEL   0x96
#define BOS_KEY_NUMLOCK 0x97
#define BOS_KEY_CTRL  0x98
#define BOS_KEY_ALT   0x99
#define BOS_KEY_SHIFT 0x9A
// Key Event Structure
typedef struct {
    uint8_t keycode;    // BOS_KEY_* or ASCII value if printable
    char ascii;         // ASCII character if printable, else 0
    uint8_t pressed;    // 1 for KeyDown, 0 for KeyUp
    uint8_t shift;      // 1 if Shift is held
    uint8_t ctrl;       // 1 if Ctrl is held
    uint8_t alt;        // 1 if Alt is held
    uint8_t caps_lock;  // 1 if Caps Lock is ON
} bos_key_event_t;

// Core System Calls
void bos_print(const char* str);
void bos_clear_screen(void);
void bos_set_cursor(uint16_t x, uint16_t y);
void bos_surface_present(uint32_t window_id, const uint32_t* pixels, uint32_t w, uint32_t h);
void bos_exit(void);
void bos_yield(void);
uint32_t bos_uptime(void);
char bos_getc(void);
int bos_get_key_event(bos_key_event_t* event);
int bos_get_input_event(void* event);

// Process System Calls
uint64_t bos_spawn(const char* path);

// File System Calls
int bos_open(const char* path);
int bos_read(int fd, void* buffer, size_t size);
int bos_write(int fd, const void* buffer, size_t size);
int bos_close(int fd);
int bos_seek(int fd, uint64_t offset, int whence);
int bos_readdir(const char* path, int index, bos_dirent_t* out_entry);
int bos_mkdir(const char* path);
int bos_create(const char* path);
int bos_rename(const char* old_path, const char* new_name);
int bos_delete(const char* path);

// Debug System Calls
void bos_heapinfo(void);
void bos_ps(void);
void bos_heapdump(void);
void bos_memmap(void);
void bos_dmesg(void);
void bos_taskinfo(int pid);
void bos_stressheap(void);
void bos_heapvalidate(void);
void bos_heapwalk(void);
void bos_heaptrace_toggle(void);

#endif
