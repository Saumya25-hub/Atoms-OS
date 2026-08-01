#include "../include/botree_nav.h"
#include "../include/botree_path.h"
#include "kernel/core/lib/include/string.h"

#define MAX_NAV_SESSIONS 32

typedef struct {
    bool         active;
    BDeNavHandle handle;
    uint32_t     owner_pid;
    char         current_dir[BDE_PATH_MAX];
    char         history[BDE_HISTORY_MAX][BDE_PATH_MAX];
    int32_t      history_count;
    int32_t      history_index;
} BDeNavSessionInternal;

static BDeNavSessionInternal s_nav_sessions[MAX_NAV_SESSIONS];
static uint32_t s_next_nav_handle = 1000;

BDeNavHandle BDe_NavCreateSession(uint32_t owner_pid) {
    for (int i = 0; i < MAX_NAV_SESSIONS; i++) {
        if (!s_nav_sessions[i].active) {
            memset(&s_nav_sessions[i], 0, sizeof(BDeNavSessionInternal));
            s_nav_sessions[i].active = true;
            s_nav_sessions[i].handle = s_next_nav_handle++;
            s_nav_sessions[i].owner_pid = owner_pid;
            strcpy(s_nav_sessions[i].current_dir, "/");
            strcpy(s_nav_sessions[i].history[0], "/");
            s_nav_sessions[i].history_count = 1;
            s_nav_sessions[i].history_index = 0;
            return s_nav_sessions[i].handle;
        }
    }
    return 0;
}

static BDeNavSessionInternal* get_session(BDeNavHandle handle) {
    if (handle == 0) return NULL;
    for (int i = 0; i < MAX_NAV_SESSIONS; i++) {
        if (s_nav_sessions[i].active && s_nav_sessions[i].handle == handle) {
            return &s_nav_sessions[i];
        }
    }
    return NULL;
}

void BDe_NavDestroySession(BDeNavHandle handle) {
    BDeNavSessionInternal* sess = get_session(handle);
    if (sess) {
        sess->active = false;
    }
}

int32_t BDe_NavOpen(BDeNavHandle handle, const char* path) {
    BDeNavSessionInternal* sess = get_session(handle);
    if (!sess || !path) return -1;

    char canonical[BDE_PATH_MAX];
    if (BDe_PathCanonicalize(sess->current_dir, path, canonical, BDE_PATH_MAX) != 0) return -1;

    strcpy(sess->current_dir, canonical);

    // Push into history ring buffer
    if (sess->history_count == 0 || !BDe_PathEquals(sess->history[sess->history_index], canonical)) {
        if (sess->history_index < BDE_HISTORY_MAX - 1) {
            sess->history_index++;
            strcpy(sess->history[sess->history_index], canonical);
            sess->history_count = sess->history_index + 1;
        } else {
            // Shift left
            for (int i = 0; i < BDE_HISTORY_MAX - 1; i++) {
                strcpy(sess->history[i], sess->history[i + 1]);
            }
            strcpy(sess->history[BDE_HISTORY_MAX - 1], canonical);
            sess->history_index = BDE_HISTORY_MAX - 1;
            sess->history_count = BDE_HISTORY_MAX;
        }
    }
    return 0;
}

int32_t BDe_NavBack(BDeNavHandle handle, char* out_path, size_t max_len) {
    BDeNavSessionInternal* sess = get_session(handle);
    if (!sess || sess->history_index <= 0) return -1;

    sess->history_index--;
    strcpy(sess->current_dir, sess->history[sess->history_index]);
    if (out_path && max_len > 0) {
        strcpy(out_path, sess->current_dir);
    }
    return 0;
}

int32_t BDe_NavForward(BDeNavHandle handle, char* out_path, size_t max_len) {
    BDeNavSessionInternal* sess = get_session(handle);
    if (!sess || sess->history_index + 1 >= sess->history_count) return -1;

    sess->history_index++;
    strcpy(sess->current_dir, sess->history[sess->history_index]);
    if (out_path && max_len > 0) {
        strcpy(out_path, sess->current_dir);
    }
    return 0;
}

int32_t BDe_NavUp(BDeNavHandle handle, char* out_path, size_t max_len) {
    BDeNavSessionInternal* sess = get_session(handle);
    if (!sess) return -1;

    char parent[BDE_PATH_MAX];
    if (BDe_PathGetDirname(sess->current_dir, parent, BDE_PATH_MAX) != 0) return -1;

    return BDe_NavOpen(handle, parent);
}

const char* BDe_NavGetCurrentDir(BDeNavHandle handle) {
    BDeNavSessionInternal* sess = get_session(handle);
    return sess ? sess->current_dir : "/";
}

bool BDe_NavCanBack(BDeNavHandle handle) {
    BDeNavSessionInternal* sess = get_session(handle);
    return sess && sess->history_index > 0;
}

bool BDe_NavCanForward(BDeNavHandle handle) {
    BDeNavSessionInternal* sess = get_session(handle);
    return sess && sess->history_index + 1 < sess->history_count;
}
