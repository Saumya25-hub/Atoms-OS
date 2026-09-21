/*
 * ATOMS OS — Lightweight ZIP / JAR Archive Engine
 * Copyright © 2026 ATOMS OS Project / Saumya Chaudhari
 *
 * Implements standard PKZIP 2.0 / JAR parsing:
 * - End of Central Directory (EOCD) locator
 * - Central Directory table reader
 * - Uncompressed (Stored, Method 0) extraction
 * - Deflate (Method 8) decompressed streaming extraction
 * - META-INF/MANIFEST.MF Main-Class parser
 * - Arbitrary resource / asset retrieval
 */

#ifndef AVIAN_ZIP_H
#define AVIAN_ZIP_H

#include <avian/common.h>
#include <avian/system/system.h>

namespace avian {
namespace zip {

// Standard ZIP Signatures
static const uint32_t LocalFileHeaderSignature = 0x04034b50;
static const uint32_t CentralDirectorySignature = 0x02014b50;
static const uint32_t EndOfCentralDirectorySig  = 0x06054b50;

struct ZipEntry {
  const char* filename;
  uint16_t filename_length;
  uint32_t uncompressed_size;
  uint32_t compressed_size;
  uint16_t compression_method;
  uint32_t local_header_offset;
  const uint8_t* data;
};

class ZipArchive {
 public:
  ZipArchive();
  ~ZipArchive();

  static bool open(system::System* sys, const uint8_t* zip_data, size_t zip_size, ZipArchive** out_archive);

  const ZipEntry* findEntry(const char* name) const;
  bool extractEntry(system::System* sys, const ZipEntry* entry, const uint8_t** out_data, size_t* out_size, bool* out_allocated) const;
  bool getMainClass(char* out_buffer, size_t max_len) const;

  uint16_t entryCount() const { return entry_count_; }
  const ZipEntry* getEntry(uint16_t index) const;

  void dispose(system::System* sys);

 private:
  const uint8_t* data_;
  size_t size_;
  uint16_t entry_count_;
  ZipEntry* entries_;
};

// Safe standalone Inflate / Deflate decompression helper
bool inflateData(const uint8_t* compressed_data, size_t compressed_size,
                 uint8_t* uncompressed_data, size_t uncompressed_size);

} // namespace zip
} // namespace avian

#endif // AVIAN_ZIP_H
