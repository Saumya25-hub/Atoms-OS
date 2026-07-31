#ifndef BOS_TYPES_H
#define BOS_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* Result / Status Return Codes */
typedef uint32_t BOS_Result;

#define BOS_SUCCESS                      0x00000000U
#define BOS_ERROR_GENERIC                0x80000001U
#define BOS_ERROR_INVALID_HANDLE         0x80000002U
#define BOS_ERROR_INVALID_ARGUMENT       0x80000003U
#define BOS_ERROR_INVALID_ADDRESS        0x80000004U
#define BOS_ERROR_OUT_OF_MEMORY          0x80000005U
#define BOS_ERROR_QUEUE_FULL             0x80000006U
#define BOS_ERROR_QUEUE_EMPTY            0x80000007U
#define BOS_ERROR_NOT_FOUND              0x80000008U
#define BOS_ERROR_ALREADY_EXISTS         0x80000009U
#define BOS_ERROR_PERMISSION_DENIED      0x8000000AU
#define BOS_ERROR_NOT_IMPLEMENTED        0x8000000BU

/* Opaque Window Handle Type (32-bit generation index handle) */
typedef uint32_t BOS_WindowHandle;
#define BOS_INVALID_WINDOW_HANDLE 0U

/* Opaque Application Handle Type */
typedef uint32_t BOS_AppHandle;
#define BOS_INVALID_APP_HANDLE 0U

/* Geometry Primitives */
typedef struct {
    int32_t x;
    int32_t y;
} BOS_Point;

typedef struct {
    uint32_t width;
    uint32_t height;
} BOS_Size;

typedef struct {
    int32_t  x;
    int32_t  y;
    uint32_t width;
    uint32_t height;
} BOS_Rect;

#endif /* BOS_TYPES_H */
