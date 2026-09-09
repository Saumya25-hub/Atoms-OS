/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef MOJO_PUBLIC_C_SYSTEM_TYPES_H_
#define MOJO_PUBLIC_C_SYSTEM_TYPES_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t MojoHandle;
#define MOJO_HANDLE_INVALID ((MojoHandle)0)

typedef uint32_t MojoResult;
#define MOJO_RESULT_OK                  ((MojoResult)0)
#define MOJO_RESULT_CANCELLED           ((MojoResult)1)
#define MOJO_RESULT_UNKNOWN             ((MojoResult)2)
#define MOJO_RESULT_INVALID_ARGUMENT    ((MojoResult)3)
#define MOJO_RESULT_DEADLINE_EXCEEDED   ((MojoResult)4)
#define MOJO_RESULT_NOT_FOUND           ((MojoResult)5)
#define MOJO_RESULT_ALREADY_EXISTS      ((MojoResult)6)
#define MOJO_RESULT_PERMISSION_DENIED   ((MojoResult)7)
#define MOJO_RESULT_RESOURCE_EXHAUSTED  ((MojoResult)8)
#define MOJO_RESULT_FAILED_PRECONDITION ((MojoResult)9)
#define MOJO_RESULT_ABORTED             ((MojoResult)10)
#define MOJO_RESULT_OUT_OF_RANGE        ((MojoResult)11)
#define MOJO_RESULT_UNIMPLEMENTED       ((MojoResult)12)
#define MOJO_RESULT_INTERNAL            ((MojoResult)13)
#define MOJO_RESULT_UNAVAILABLE         ((MojoResult)14)
#define MOJO_RESULT_DATA_LOSS           ((MojoResult)15)
#define MOJO_RESULT_BUSY                ((MojoResult)16)
#define MOJO_RESULT_SHOULD_WAIT         ((MojoResult)17)

typedef uint32_t MojoHandleSignals;
#define MOJO_HANDLE_SIGNAL_NONE         ((MojoHandleSignals)0)
#define MOJO_HANDLE_SIGNAL_READABLE     ((MojoHandleSignals)(1 << 0))
#define MOJO_HANDLE_SIGNAL_WRITABLE     ((MojoHandleSignals)(1 << 1))
#define MOJO_HANDLE_SIGNAL_PEER_CLOSED  ((MojoHandleSignals)(1 << 2))
#define MOJO_HANDLE_SIGNAL_PEER_REMOTE  ((MojoHandleSignals)(1 << 3))

typedef uint32_t MojoHandleRights;
#define MOJO_HANDLE_RIGHT_NONE          ((MojoHandleRights)0)
#define MOJO_HANDLE_RIGHT_DUPLICATE     ((MojoHandleRights)(1 << 0))
#define MOJO_HANDLE_RIGHT_TRANSFER      ((MojoHandleRights)(1 << 1))
#define MOJO_HANDLE_RIGHT_READ          ((MojoHandleRights)(1 << 2))
#define MOJO_HANDLE_RIGHT_WRITE         ((MojoHandleRights)(1 << 3))
#define MOJO_HANDLE_RIGHT_EXECUTE       ((MojoHandleRights)(1 << 4))
#define MOJO_HANDLE_RIGHT_MAP_READ      ((MojoHandleRights)(1 << 5))
#define MOJO_HANDLE_RIGHT_MAP_WRITE     ((MojoHandleRights)(1 << 6))
#define MOJO_HANDLE_RIGHT_MAP_EXECUTE   ((MojoHandleRights)(1 << 7))

typedef uint32_t MojoHandleType;
#define MOJO_HANDLE_TYPE_INVALID        ((MojoHandleType)0)
#define MOJO_HANDLE_TYPE_MESSAGE_PIPE   ((MojoHandleType)1)
#define MOJO_HANDLE_TYPE_SHARED_BUFFER  ((MojoHandleType)2)
#define MOJO_HANDLE_TYPE_DATA_PIPE      ((MojoHandleType)3)

#ifdef __cplusplus
}
#endif

#endif // MOJO_PUBLIC_C_SYSTEM_TYPES_H_
