/*
 * ATOMS OS — Storage VFS Adapter
 * Bridges Chromium Storage to ATOMS VFS for persistent localStorage
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 *
 * Uses real ATOMS kernel VFS API:
 *   ATOMS_VFS_OpenFile  → open file by path
 *   ATOMS_VFS_WriteFile → write bytes
 *   ATOMS_VFS_ReadFile  → read bytes
 *   ATOMS_VFS_CloseFile → close handle
 *
 * Storage file format:
 *   [4 bytes]  Magic: 0x53544F52 ("STOR")
 *   [4 bytes]  Version: 1
 *   [4 bytes]  Entry count
 *   [4 bytes]  CRC32 of all entry data
 *   For each entry:
 *     [4 bytes] Key length (uint32_t)
 *     [N bytes] Key data (UTF-8)
 *     [4 bytes] Value length (uint32_t)
 *     [N bytes] Value data (UTF-8)
 */

#include "atoms_storage_vfs_adapter.h"
#include <stdint.h>

#include "userspace/runtime/c/include/unistd.h"

// ATOMS kernel VFS C ABI
extern "C" {
#include "kernel/application/future_vfs_api/app_vfs_api.h"

__attribute__((weak)) int32_t ATOMS_VFS_OpenFile(uint32_t app_id, const char* path, const char* mode) {
    (void)app_id;
    int flags = 0;
    if (mode && mode[0] == 'w') flags = 1;
    return open(path, flags, 0644);
}

__attribute__((weak)) int32_t ATOMS_VFS_ReadFile(int32_t handle, void* buffer, uint32_t size) {
    return (int32_t)read(handle, buffer, size);
}

__attribute__((weak)) int32_t ATOMS_VFS_WriteFile(int32_t handle, const void* buffer, uint32_t size) {
    return (int32_t)write(handle, buffer, size);
}

__attribute__((weak)) void ATOMS_VFS_CloseFile(int32_t handle) {
    close(handle);
}

__attribute__((weak)) bool ATOMS_VFS_ListDirectory(uint32_t app_id, const char* path, ATOMS_VFS_FileInfo* out_entries, uint32_t max_entries, uint32_t* out_count) {
    (void)app_id; (void)path; (void)out_entries; (void)max_entries;
    if (out_count) *out_count = 0;
    return true;
}
}

namespace storage {

// App ID for ATRIX browser storage
static constexpr uint32_t ATRIX_APP_ID = 1;

// Simple hash for origin → filename
static uint32_t SimpleOriginHash(const std::string& origin_str) {
    uint32_t hash = 5381;
    for (size_t i = 0; i < origin_str.size(); i++) {
        hash = ((hash << 5) + hash) + (uint8_t)origin_str[i];
    }
    return hash;
}

std::string AtomsStorageVFS_GetPath(const net::SecurityOrigin& origin) {
    uint32_t hash = SimpleOriginHash(origin.ToString());
    char path[128];
    // Format: /var/storage/local_XXXXXXXX.dat
    int len = 0;
    const char* prefix = "/var/storage/local_";
    for (int i = 0; prefix[i]; i++) path[len++] = prefix[i];

    // Convert hash to hex
    const char* hex = "0123456789abcdef";
    for (int i = 7; i >= 0; i--) {
        path[len++] = hex[(hash >> (i * 4)) & 0xF];
    }
    path[len++] = '.';
    path[len++] = 'd';
    path[len++] = 'a';
    path[len++] = 't';
    path[len] = '\0';

    return std::string(path, len);
}

// CRC32 implementation (standard polynomial 0xEDB88320)
uint32_t AtomsStorageVFS_CRC32(const uint8_t* data, size_t len) {
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 1) {
                crc = (crc >> 1) ^ 0xEDB88320;
            } else {
                crc >>= 1;
            }
        }
    }
    return ~crc;
}

static void WriteU32(uint8_t* buf, uint32_t val) {
    buf[0] = (uint8_t)(val & 0xFF);
    buf[1] = (uint8_t)((val >> 8) & 0xFF);
    buf[2] = (uint8_t)((val >> 16) & 0xFF);
    buf[3] = (uint8_t)((val >> 24) & 0xFF);
}

static uint32_t ReadU32(const uint8_t* buf) {
    return (uint32_t)buf[0] | ((uint32_t)buf[1] << 8) |
           ((uint32_t)buf[2] << 16) | ((uint32_t)buf[3] << 24);
}

bool AtomsStorageVFS_Save(const net::SecurityOrigin& origin,
                          const std::vector<StorageEntry>& entries) {
    std::string path = AtomsStorageVFS_GetPath(origin);

    // 1. Serialize entries into a buffer
    // Calculate total size
    size_t entry_data_size = 0;
    for (size_t i = 0; i < entries.size(); i++) {
        entry_data_size += 4 + entries[i].key.size() + 4 + entries[i].value.size();
    }

    // Header: magic(4) + version(4) + count(4) + crc(4) = 16 bytes
    size_t total_size = 16 + entry_data_size;
    uint8_t* buf = new uint8_t[total_size];
    if (!buf) return false;

    // 2. Write header
    WriteU32(buf + 0, ATOMS_STORAGE_MAGIC);
    WriteU32(buf + 4, ATOMS_STORAGE_VERSION);
    WriteU32(buf + 8, (uint32_t)entries.size());
    // CRC placeholder at offset 12 — will be filled after serializing entries

    // 3. Serialize entries
    size_t offset = 16;
    for (size_t i = 0; i < entries.size(); i++) {
        uint32_t key_len = (uint32_t)entries[i].key.size();
        uint32_t val_len = (uint32_t)entries[i].value.size();

        WriteU32(buf + offset, key_len);
        offset += 4;
        for (size_t j = 0; j < key_len; j++) buf[offset++] = entries[i].key[j];

        WriteU32(buf + offset, val_len);
        offset += 4;
        for (size_t j = 0; j < val_len; j++) buf[offset++] = entries[i].value[j];
    }

    // 4. Compute CRC32 of entry data (bytes 16..end)
    uint32_t crc = AtomsStorageVFS_CRC32(buf + 16, entry_data_size);
    WriteU32(buf + 12, crc);

    // 5. Write to ATOMS VFS
    int32_t fd = ATOMS_VFS_OpenFile(ATRIX_APP_ID, path.c_str(), "wb");
    if (fd < 0) {
        delete[] buf;
        return false;
    }

    int32_t written = ATOMS_VFS_WriteFile(fd, buf, (uint32_t)total_size);
    ATOMS_VFS_CloseFile(fd);
    delete[] buf;

    return written == (int32_t)total_size;
}

bool AtomsStorageVFS_Load(const net::SecurityOrigin& origin,
                          std::vector<StorageEntry>* out_entries) {
    if (!out_entries) return false;
    out_entries->clear();

    std::string path = AtomsStorageVFS_GetPath(origin);

    // 1. Open file
    int32_t fd = ATOMS_VFS_OpenFile(ATRIX_APP_ID, path.c_str(), "rb");
    if (fd < 0) return false; // File doesn't exist — not an error

    // 2. Read header (16 bytes)
    uint8_t header[16];
    int32_t hdr_read = ATOMS_VFS_ReadFile(fd, header, 16);
    if (hdr_read != 16) {
        ATOMS_VFS_CloseFile(fd);
        return false;
    }

    // 3. Validate magic
    uint32_t magic = ReadU32(header + 0);
    if (magic != ATOMS_STORAGE_MAGIC) {
        ATOMS_VFS_CloseFile(fd);
        return false;
    }

    // 4. Validate version
    uint32_t version = ReadU32(header + 4);
    if (version != ATOMS_STORAGE_VERSION) {
        ATOMS_VFS_CloseFile(fd);
        return false;
    }

    uint32_t entry_count = ReadU32(header + 8);
    uint32_t stored_crc = ReadU32(header + 12);

    // 5. Read all entry data
    // We need to read the rest of the file. Read in chunks.
    std::string entry_data = "";
    uint8_t chunk[4096];
    while (true) {
        int32_t r = ATOMS_VFS_ReadFile(fd, chunk, sizeof(chunk));
        if (r <= 0) break;
        entry_data.append((const char*)chunk, r);
    }
    ATOMS_VFS_CloseFile(fd);

    // 6. Verify CRC32
    uint32_t computed_crc = AtomsStorageVFS_CRC32((const uint8_t*)entry_data.c_str(),
                                                   entry_data.size());
    if (computed_crc != stored_crc) {
        return false; // Corrupted data
    }

    // 7. Deserialize entries
    const uint8_t* data = (const uint8_t*)entry_data.c_str();
    size_t data_len = entry_data.size();
    size_t offset = 0;

    for (uint32_t i = 0; i < entry_count; i++) {
        if (offset + 4 > data_len) return false;
        uint32_t key_len = ReadU32(data + offset);
        offset += 4;

        if (offset + key_len > data_len) return false;
        std::string key((const char*)(data + offset), key_len);
        offset += key_len;

        if (offset + 4 > data_len) return false;
        uint32_t val_len = ReadU32(data + offset);
        offset += 4;

        if (offset + val_len > data_len) return false;
        std::string value((const char*)(data + offset), val_len);
        offset += val_len;

        StorageEntry entry;
        entry.key = key;
        entry.value = value;
        out_entries->push_back(entry);
    }

    return true;
}

bool AtomsStorageVFS_Delete(const net::SecurityOrigin& origin) {
    std::string path = AtomsStorageVFS_GetPath(origin);

    // Write zero-length file to "delete" (ATOMS VFS may not support unlink yet)
    int32_t fd = ATOMS_VFS_OpenFile(ATRIX_APP_ID, path.c_str(), "wb");
    if (fd < 0) return false;
    ATOMS_VFS_CloseFile(fd);
    return true;
}

} // namespace storage
