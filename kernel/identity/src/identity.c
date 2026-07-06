#include "kernel/identity/include/identity.h"
#include "kernel/core/lib/include/string.h"

extern void display_print(const char* str);

/*
 * Phase 1 Temporary Hardcoded Credentials.
 * In Phase 2 & 3, these will be dynamically retrieved from system settings / SQLite database.
 */
static const char* s_default_username = "admin";
static const char* s_default_password = "admin123";
static const char* s_display_username = "ADMINISTRATOR";

void Identity_Init(void) {
    display_print("[AIE] ATOMS Identity Engine Initializing (Phase 1: Temporary Credentials)\n");
}

bool Identity_Authenticate(const char* username, const char* password) {
    if (!username || !password) {
        display_print("[AIE] Authentication failed: NULL credential pointer\n");
        return false;
    }

    /* Check against Phase 1 temporary credentials (case-insensitive for username, exact for password) */
    bool user_match = (strcmp(username, s_default_username) == 0) || (strcmp(username, s_display_username) == 0);
    bool pass_match = (strcmp(password, s_default_password) == 0);

    if (user_match && pass_match) {
        display_print("[AIE] Authentication SUCCESS for account: admin\n");
        return true;
    } else {
        display_print("[AIE] Authentication FAILURE: Invalid username or password\n");
        return false;
    }
}

const char* Identity_GetDefaultUsername(void) {
    return s_display_username;
}
