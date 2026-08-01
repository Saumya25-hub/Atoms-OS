#ifndef BOS_ADVAPI32_TYPES_H
#define BOS_ADVAPI32_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "userspace/libs/kernel32/include/kernel32_types.h"

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

typedef uint32_t* LPDWORD;
typedef uint32_t* PDWORD;
typedef int32_t*  PBOOL;
typedef void*     PVOID;
typedef uint8_t*  LPBYTE;

typedef struct _LUID {
    uint32_t LowPart;
    int32_t  HighPart;
} LUID, *PLUID;

typedef HANDLE HKEY;
typedef HANDLE SC_HANDLE;
typedef HANDLE HCRYPTPROV;
typedef HANDLE HCRYPTHASH;
typedef HANDLE HCRYPTKEY;
typedef void*  PSID;
typedef void*  PACL;
typedef void*  PSECURITY_DESCRIPTOR;

#define HKEY_CLASSES_ROOT                  ((HKEY)(uintptr_t)0x80000000)
#define HKEY_CURRENT_USER                  ((HKEY)(uintptr_t)0x80000001)
#define HKEY_LOCAL_MACHINE                 ((HKEY)(uintptr_t)0x80000002)
#define HKEY_USERS                         ((HKEY)(uintptr_t)0x80000003)

#define REG_OPTION_NON_VOLATILE            0x00000000
#define REG_CREATED_NEW_KEY                0x00000001
#define REG_OPENED_EXISTING_KEY            0x00000002

#define REG_NONE                           0
#define REG_SZ                             1
#define REG_EXPAND_SZ                      2
#define REG_BINARY                         3
#define REG_DWORD                          4
#define REG_MULTI_SZ                       7

#define KEY_READ                           0x20019
#define KEY_WRITE                          0x20006
#define KEY_ALL_ACCESS                     0xF003F

#define SC_MANAGER_ALL_ACCESS              0xF003F
#define SERVICE_ALL_ACCESS                 0xF01FF
#define SERVICE_WIN32_OWN_PROCESS          0x00000010
#define SERVICE_AUTO_START                 0x00000002
#define SERVICE_ERROR_NORMAL               0x00000001

#define PROV_RSA_FULL                      1
#define CRYPT_VERIFYCONTEXT                0xF0000000
#define CALG_SHA1                          0x00008004

typedef struct _SID_IDENTIFIER_AUTHORITY {
    BYTE Value[6];
} SID_IDENTIFIER_AUTHORITY, *PSID_IDENTIFIER_AUTHORITY;

typedef struct _TOKEN_PRIVILEGES {
    DWORD PrivilegeCount;
    struct {
        LUID Luid;
        DWORD Attributes;
    } Privileges[1];
} TOKEN_PRIVILEGES, *PTOKEN_PRIVILEGES;

#endif // BOS_ADVAPI32_TYPES_H
