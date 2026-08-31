/*
 * ATOMS OS — Storage VFS Adapter Header
 * Bridges Chromium Storage to ATOMS VFS for persistent localStorage
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 *
 * Storage file format:
 *   /var/storage/local_<origin_hash>.dat
 *
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

#ifndef THIRD_PARTY_CHROMIUM_STORAGE_ADAPTER_ATOMS_STORAGE_VFS_ADAPTER_H_
#define THIRD_PARTY_CHROMIUM_STORAGE_ADAPTER_ATOMS_STORAGE_VFS_ADAPTER_H_

#include "third_party/chromium_net/base/security_origin.h"
#include "third_party/chromium_storage/dom_storage/storage_area.h"
#include "userspace/runtime/cpp/include/string"
#include "userspace/runtime/cpp/include/vector"

namespace storage {

#define ATOMS_STORAGE_MAGIC 0x53544F52  // "STOR"
#define ATOMS_STORAGE_VERSION 1

/*
 * Compute the VFS file path for an origin.
 * Format: /var/storage/local_<origin_hash>.dat
 */
std::string AtomsStorageVFS_GetPath(const net::SecurityOrigin& origin);

/*
 * Save a StorageArea's entries to ATOMS VFS disk file.
 * Writes magic header + version + entry count + CRC32 + entries.
 */
bool AtomsStorageVFS_Save(const net::SecurityOrigin& origin,
                          const std::vector<StorageEntry>& entries);

/*
 * Load a StorageArea's entries from ATOMS VFS disk file.
 * Validates magic header, version, and CRC32 before returning data.
 */
bool AtomsStorageVFS_Load(const net::SecurityOrigin& origin,
                          std::vector<StorageEntry>* out_entries);

/*
 * Delete the VFS storage file for an origin.
 */
bool AtomsStorageVFS_Delete(const net::SecurityOrigin& origin);

/*
 * CRC32 helper for data integrity verification.
 */
uint32_t AtomsStorageVFS_CRC32(const uint8_t* data, size_t len);

} // namespace storage

#endif // THIRD_PARTY_CHROMIUM_STORAGE_ADAPTER_ATOMS_STORAGE_VFS_ADAPTER_H_
