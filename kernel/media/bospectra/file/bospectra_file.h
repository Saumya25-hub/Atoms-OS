#ifndef BOSPECTRA_FILE_H
#define BOSPECTRA_FILE_H

#include "../include/bospectra_types.h"

typedef struct {
    bospectra_file_id_t id;
    int                 vfs_fd;
    char                path[BOSPECTRA_MAX_PATH_LEN];
    uint64_t            file_size;
    uint64_t            current_offset;
    bool                is_open;
} BOSPECTRA_FileContext;

void              bospectra_file_subsystem_init(void);
void              bospectra_file_subsystem_shutdown(void);
bospectra_error_t bospectra_file_open(const char* path, bospectra_file_id_t* out_file_id);
bospectra_error_t bospectra_file_read(bospectra_file_id_t file_id, void* buffer, uint32_t bytes_to_read, uint32_t* out_bytes_read);
bospectra_error_t bospectra_file_seek(bospectra_file_id_t file_id, uint64_t target_offset);
bospectra_error_t bospectra_file_tell(bospectra_file_id_t file_id, uint64_t* out_offset);
bospectra_error_t bospectra_file_get_size(bospectra_file_id_t file_id, uint64_t* out_size);
bospectra_error_t bospectra_file_close(bospectra_file_id_t file_id);

#endif // BOSPECTRA_FILE_H
