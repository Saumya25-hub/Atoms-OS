#ifndef BOS_EVENTS_H
#define BOS_EVENTS_H

#include "bos_types.h"

typedef enum {
    BOS_EVENT_NONE = 0,
    BOS_EVENT_MOUSE_MOVE,
    BOS_EVENT_MOUSE_CLICK,
    BOS_EVENT_MOUSE_WHEEL,
    BOS_EVENT_KEYBOARD,
    BOS_EVENT_WINDOW_MOVE,
    BOS_EVENT_WINDOW_RESIZE,
    BOS_EVENT_WINDOW_FOCUS,
    BOS_EVENT_WINDOW_CLOSE,
    BOS_EVENT_PAINT,
    BOS_EVENT_CUSTOM
} BOS_EventType;

typedef struct {
    BOS_EventType type;
    uint32_t      target_window;
    uint64_t      timestamp_us;
    union {
        struct { int32_t x; int32_t y; int32_t delta_x; int32_t delta_y; } mouse_move;
        struct { int32_t x; int32_t y; uint8_t button; bool pressed; } mouse_click;
        struct { int32_t x; int32_t y; int32_t delta; } mouse_wheel;
        struct { uint32_t key_code; uint32_t char_code; bool pressed; } key;
        struct { int32_t x; int32_t y; uint32_t width; uint32_t height; } bounds;
        struct { uint32_t custom_id; uintptr_t arg1; uintptr_t arg2; } custom;
    } data;
} BOS_Event;

#define BOS_EVENT_QUEUE_CAPACITY 256U

typedef struct {
    BOS_Event events[BOS_EVENT_QUEUE_CAPACITY];
    volatile uint32_t head;
    volatile uint32_t tail;
    volatile uint32_t dropped_events;
    volatile uint32_t coalesced_events;
} BOS_EventQueue;

void       BOS_EventQueue_Init(BOS_EventQueue* queue);
BOS_Result BOS_EventQueue_Push(BOS_EventQueue* queue, const BOS_Event* event);
BOS_Result BOS_EventQueue_Pop(BOS_EventQueue* queue, BOS_Event* out_event);
BOS_Result BOS_EventQueue_Peek(BOS_EventQueue* queue, BOS_Event* out_event);
bool       BOS_EventQueue_IsEmpty(const BOS_EventQueue* queue);
bool       BOS_EventQueue_IsFull(const BOS_EventQueue* queue);

#endif /* BOS_EVENTS_H */
