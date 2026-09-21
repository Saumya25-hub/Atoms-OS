/*
 * ATOMS OS — Lightweight ZIP / JAR Archive Engine Implementation
 * Copyright © 2026 ATOMS OS Project / Saumya Chaudhari
 *
 * Implements standard PKZIP / JAR parsing & safe standalone Deflate / Inflate:
 * - Reverse scanning for End of Central Directory (EOCD, 0x06054b50)
 * - Traversal of Central Directory entries (0x02014b50)
 * - Local File Header payload resolution (0x04034b50)
 * - MANIFEST.MF Main-Class attribute extraction
 * - Method 0 (Stored) & Method 8 (Deflate / Inflate decompression)
 */

#include <avian/zip.h>
#include <userspace/runtime/cpp/include/new>
#include <string.h>

namespace avian {
namespace zip {

static uint16_t readU16LE(const uint8_t* p) {
  return static_cast<uint16_t>(p[0]) | (static_cast<uint16_t>(p[1]) << 8);
}

static uint32_t readU32LE(const uint8_t* p) {
  return static_cast<uint32_t>(p[0]) |
         (static_cast<uint32_t>(p[1]) << 8) |
         (static_cast<uint32_t>(p[2]) << 16) |
         (static_cast<uint32_t>(p[3]) << 24);
}

static bool strEqualsLen(const char* s1, const char* s2, size_t len) {
  for (size_t i = 0; i < len; ++i) {
    if (s1[i] != s2[i]) return false;
    if (s1[i] == '\0') return true;
  }
  return s1[len] == '\0';
}

static size_t strLen(const char* s) {
  size_t len = 0;
  while (s[len]) ++len;
  return len;
}

// ---------------------------------------------------------------------------
// Standalone Safe Inflate (RFC 1951 Deflate Decompressor)
// ---------------------------------------------------------------------------

namespace {

class BitStream {
 public:
  BitStream(const uint8_t* data, size_t size)
      : data_(data), size_(size), bit_pos_(0) {}

  bool readBits(uint32_t num_bits, uint32_t* out_val) {
    if (num_bits == 0) {
      if (out_val) *out_val = 0;
      return true;
    }
    uint32_t res = 0;
    for (uint32_t i = 0; i < num_bits; ++i) {
      size_t byte_idx = (bit_pos_ + i) / 8;
      uint32_t bit_idx = (bit_pos_ + i) % 8;
      if (byte_idx >= size_) return false;
      uint32_t bit = (data_[byte_idx] >> bit_idx) & 1;
      res |= (bit << i);
    }
    bit_pos_ += num_bits;
    if (out_val) *out_val = res;
    return true;
  }

  void alignToByte() {
    bit_pos_ = (bit_pos_ + 7) & ~7ULL;
  }

  size_t currentByteOffset() const {
    return bit_pos_ / 8;
  }

  bool hasMoreBits() const {
    return bit_pos_ < size_ * 8;
  }

 private:
  const uint8_t* data_;
  size_t size_;
  size_t bit_pos_;
};

struct HuffmanCode {
  uint16_t code;
  uint8_t len;
  uint16_t symbol;
};

struct HuffmanTree {
  HuffmanCode codes[320];
  uint16_t count;

  bool build(const uint8_t* lengths, uint16_t num_symbols) {
    count = 0;
    uint8_t max_len = 0;
    uint16_t bl_count[16] = {0};

    for (uint16_t i = 0; i < num_symbols; ++i) {
      if (lengths[i] > 15) return false;
      if (lengths[i] > 0) {
        bl_count[lengths[i]]++;
        if (lengths[i] > max_len) max_len = lengths[i];
      }
    }

    uint16_t next_code[16] = {0};
    uint16_t code = 0;
    for (uint8_t bits = 1; bits <= 15; ++bits) {
      code = (code + bl_count[bits - 1]) << 1;
      next_code[bits] = code;
    }

    for (uint16_t i = 0; i < num_symbols; ++i) {
      uint8_t len = lengths[i];
      if (len != 0) {
        codes[count].code = next_code[len]++;
        codes[count].len = len;
        codes[count].symbol = i;
        count++;
      }
    }
    return true;
  }

  bool decode(BitStream* bs, uint16_t* out_symbol) const {
    uint32_t cur_code = 0;
    for (uint8_t bits = 1; bits <= 15; ++bits) {
      uint32_t next_bit = 0;
      if (!bs->readBits(1, &next_bit)) return false;
      cur_code = (cur_code << 1) | next_bit;
      for (uint16_t i = 0; i < count; ++i) {
        if (codes[i].len == bits && codes[i].code == cur_code) {
          if (out_symbol) *out_symbol = codes[i].symbol;
          return true;
        }
      }
    }
    return false;
  }
};

static const uint16_t kLengthBases[29] = {
    3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31,
    35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258
};
static const uint8_t kLengthExtraBits[29] = {
    0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2,
    3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0
};

static const uint16_t kDistBases[30] = {
    1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193,
    257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577
};
static const uint8_t kDistExtraBits[30] = {
    0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6,
    7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13
};

static const uint8_t kCodeOrder[19] = {
    16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15
};

} // anonymous namespace

bool inflateData(const uint8_t* comp_data, size_t comp_size,
                 uint8_t* uncomp_data, size_t uncomp_size) {
  if (!comp_data || comp_size == 0 || !uncomp_data || uncomp_size == 0) {
    return false;
  }

  BitStream bs(comp_data, comp_size);
  size_t out_pos = 0;
  bool bfinal = false;

  while (!bfinal && bs.hasMoreBits()) {
    uint32_t bfinal_val = 0;
    uint32_t btype_val = 0;
    if (!bs.readBits(1, &bfinal_val)) return false;
    if (!bs.readBits(2, &btype_val)) return false;
    bfinal = (bfinal_val != 0);

    if (btype_val == 0) {
      // Uncompressed block
      bs.alignToByte();
      size_t byte_pos = bs.currentByteOffset();
      if (byte_pos + 4 > comp_size) return false;
      uint16_t len = readU16LE(comp_data + byte_pos);
      uint16_t nlen = readU16LE(comp_data + byte_pos + 2);
      if (static_cast<uint16_t>(len ^ 0xFFFF) != nlen) return false;
      byte_pos += 4;
      if (byte_pos + len > comp_size || out_pos + len > uncomp_size) return false;
      memcpy(uncomp_data + out_pos, comp_data + byte_pos, len);
      out_pos += len;
      // advance bitstream
      for (size_t k = 0; k < (4 + len) * 8; ++k) {
        uint32_t dummy;
        bs.readBits(1, &dummy);
      }
    } else if (btype_val == 1) {
      // Fixed Huffman block
      uint8_t lit_lens[288];
      for (int i = 0; i <= 143; ++i) lit_lens[i] = 8;
      for (int i = 144; i <= 255; ++i) lit_lens[i] = 9;
      for (int i = 256; i <= 279; ++i) lit_lens[i] = 7;
      for (int i = 280; i <= 287; ++i) lit_lens[i] = 8;

      HuffmanTree lit_tree;
      if (!lit_tree.build(lit_lens, 288)) return false;

      uint8_t dist_lens[32];
      for (int i = 0; i < 32; ++i) dist_lens[i] = 5;
      HuffmanTree dist_tree;
      if (!dist_tree.build(dist_lens, 32)) return false;

      while (true) {
        uint16_t sym = 0;
        if (!lit_tree.decode(&bs, &sym)) return false;
        if (sym < 256) {
          if (out_pos >= uncomp_size) return false;
          uncomp_data[out_pos++] = static_cast<uint8_t>(sym);
        } else if (sym == 256) {
          break; // End of block
        } else if (sym <= 285) {
          uint16_t len_idx = sym - 257;
          uint32_t extra_len = 0;
          if (kLengthExtraBits[len_idx] > 0) {
            if (!bs.readBits(kLengthExtraBits[len_idx], &extra_len)) return false;
          }
          uint32_t length = kLengthBases[len_idx] + extra_len;

          uint16_t dist_sym = 0;
          if (!dist_tree.decode(&bs, &dist_sym) || dist_sym >= 30) return false;
          uint32_t extra_dist = 0;
          if (kDistExtraBits[dist_sym] > 0) {
            if (!bs.readBits(kDistExtraBits[dist_sym], &extra_dist)) return false;
          }
          uint32_t distance = kDistBases[dist_sym] + extra_dist;

          if (distance > out_pos || out_pos + length > uncomp_size) return false;
          for (uint32_t k = 0; k < length; ++k) {
            uncomp_data[out_pos] = uncomp_data[out_pos - distance];
            out_pos++;
          }
        } else {
          return false;
        }
      }
    } else if (btype_val == 2) {
      // Dynamic Huffman block
      uint32_t hlit = 0, hdist = 0, hclen = 0;
      if (!bs.readBits(5, &hlit)) return false;
      if (!bs.readBits(5, &hdist)) return false;
      if (!bs.readBits(4, &hclen)) return false;
      hlit += 257;
      hdist += 1;
      hclen += 4;

      uint8_t code_lens[19] = {0};
      for (uint32_t i = 0; i < hclen; ++i) {
        uint32_t clen = 0;
        if (!bs.readBits(3, &clen)) return false;
        code_lens[kCodeOrder[i]] = static_cast<uint8_t>(clen);
      }

      HuffmanTree cl_tree;
      if (!cl_tree.build(code_lens, 19)) return false;

      uint8_t all_lens[320] = {0};
      uint32_t total_to_read = hlit + hdist;
      uint32_t read_idx = 0;

      while (read_idx < total_to_read) {
        uint16_t sym = 0;
        if (!cl_tree.decode(&bs, &sym)) return false;
        if (sym < 16) {
          all_lens[read_idx++] = static_cast<uint8_t>(sym);
        } else if (sym == 16) {
          if (read_idx == 0) return false;
          uint32_t repeat = 0;
          if (!bs.readBits(2, &repeat)) return false;
          repeat += 3;
          uint8_t prev = all_lens[read_idx - 1];
          while (repeat-- > 0 && read_idx < total_to_read) {
            all_lens[read_idx++] = prev;
          }
        } else if (sym == 17) {
          uint32_t repeat = 0;
          if (!bs.readBits(3, &repeat)) return false;
          repeat += 3;
          while (repeat-- > 0 && read_idx < total_to_read) {
            all_lens[read_idx++] = 0;
          }
        } else if (sym == 18) {
          uint32_t repeat = 0;
          if (!bs.readBits(7, &repeat)) return false;
          repeat += 11;
          while (repeat-- > 0 && read_idx < total_to_read) {
            all_lens[read_idx++] = 0;
          }
        } else {
          return false;
        }
      }

      HuffmanTree dyn_lit_tree;
      if (!dyn_lit_tree.build(all_lens, static_cast<uint16_t>(hlit))) return false;
      HuffmanTree dyn_dist_tree;
      if (!dyn_dist_tree.build(all_lens + hlit, static_cast<uint16_t>(hdist))) return false;

      while (true) {
        uint16_t sym = 0;
        if (!dyn_lit_tree.decode(&bs, &sym)) return false;
        if (sym < 256) {
          if (out_pos >= uncomp_size) return false;
          uncomp_data[out_pos++] = static_cast<uint8_t>(sym);
        } else if (sym == 256) {
          break; // End of block
        } else if (sym <= 285) {
          uint16_t len_idx = sym - 257;
          uint32_t extra_len = 0;
          if (kLengthExtraBits[len_idx] > 0) {
            if (!bs.readBits(kLengthExtraBits[len_idx], &extra_len)) return false;
          }
          uint32_t length = kLengthBases[len_idx] + extra_len;

          uint16_t dist_sym = 0;
          if (!dyn_dist_tree.decode(&bs, &dist_sym) || dist_sym >= 30) return false;
          uint32_t extra_dist = 0;
          if (kDistExtraBits[dist_sym] > 0) {
            if (!bs.readBits(kDistExtraBits[dist_sym], &extra_dist)) return false;
          }
          uint32_t distance = kDistBases[dist_sym] + extra_dist;

          if (distance > out_pos || out_pos + length > uncomp_size) return false;
          for (uint32_t k = 0; k < length; ++k) {
            uncomp_data[out_pos] = uncomp_data[out_pos - distance];
            out_pos++;
          }
        } else {
          return false;
        }
      }
    } else {
      return false; // Reserved / invalid BTYPE
    }
  }

  return out_pos == uncomp_size;
}

ZipArchive::ZipArchive()
    : data_(nullptr), size_(0), entry_count_(0), entries_(nullptr) {}

ZipArchive::~ZipArchive() {}

void ZipArchive::dispose(system::System* sys) {
  if (entries_ && sys) {
    sys->free(entries_);
    entries_ = nullptr;
  }
  entry_count_ = 0;
  data_ = nullptr;
  size_ = 0;
}

bool ZipArchive::open(system::System* sys, const uint8_t* zip_data, size_t zip_size, ZipArchive** out_archive) {
  if (!sys || !zip_data || zip_size < 22 || !out_archive) {
    return false;
  }

  // Scan backwards for End of Central Directory Record (EOCD)
  size_t max_search = (zip_size < 65557) ? zip_size : 65557;
  size_t eocd_pos = 0;
  bool found_eocd = false;

  for (size_t offset = 22; offset <= max_search; ++offset) {
    size_t pos = zip_size - offset;
    if (readU32LE(zip_data + pos) == EndOfCentralDirectorySig) {
      eocd_pos = pos;
      found_eocd = true;
      break;
    }
  }

  if (!found_eocd) {
    return false;
  }

  const uint8_t* eocd = zip_data + eocd_pos;
  uint16_t total_entries = readU16LE(eocd + 10);
  uint32_t cd_size = readU32LE(eocd + 12);
  uint32_t cd_offset = readU32LE(eocd + 16);

  if (cd_offset + cd_size > zip_size) {
    return false;
  }

  void* mem = sys->allocate(sizeof(ZipArchive));
  if (!mem) return false;
  ZipArchive* archive = new (mem) ZipArchive();
  archive->data_ = zip_data;
  archive->size_ = zip_size;
  archive->entry_count_ = total_entries;

  if (total_entries > 0) {
    archive->entries_ = static_cast<ZipEntry*>(sys->allocate(sizeof(ZipEntry) * total_entries));
    if (!archive->entries_) {
      sys->free(archive);
      return false;
    }

    size_t cd_cursor = cd_offset;
    for (uint16_t i = 0; i < total_entries; ++i) {
      if (cd_cursor + 46 > zip_size) {
        archive->dispose(sys);
        sys->free(archive);
        return false;
      }

      if (readU32LE(zip_data + cd_cursor) != CentralDirectorySignature) {
        archive->dispose(sys);
        sys->free(archive);
        return false;
      }

      uint16_t comp_method = readU16LE(zip_data + cd_cursor + 10);
      uint32_t comp_size   = readU32LE(zip_data + cd_cursor + 20);
      uint32_t uncomp_size = readU32LE(zip_data + cd_cursor + 24);
      uint16_t fn_len      = readU16LE(zip_data + cd_cursor + 28);
      uint16_t extra_len   = readU16LE(zip_data + cd_cursor + 30);
      uint16_t comment_len = readU16LE(zip_data + cd_cursor + 32);
      uint32_t lfh_offset  = readU32LE(zip_data + cd_cursor + 42);

      const char* fn_ptr = reinterpret_cast<const char*>(zip_data + cd_cursor + 46);

      archive->entries_[i].filename = fn_ptr;
      archive->entries_[i].filename_length = fn_len;
      archive->entries_[i].compression_method = comp_method;
      archive->entries_[i].compressed_size = comp_size;
      archive->entries_[i].uncompressed_size = uncomp_size;
      archive->entries_[i].local_header_offset = lfh_offset;
      archive->entries_[i].data = nullptr;

      // Resolve payload from Local File Header
      if (lfh_offset + 30 <= zip_size &&
          readU32LE(zip_data + lfh_offset) == LocalFileHeaderSignature) {
        uint16_t lfh_fn_len = readU16LE(zip_data + lfh_offset + 26);
        uint16_t lfh_ex_len = readU16LE(zip_data + lfh_offset + 28);
        size_t payload_offset = lfh_offset + 30 + lfh_fn_len + lfh_ex_len;
        if (payload_offset + comp_size <= zip_size) {
          archive->entries_[i].data = zip_data + payload_offset;
        }
      }

      cd_cursor += 46 + fn_len + extra_len + comment_len;
    }
  }

  *out_archive = archive;
  return true;
}

const ZipEntry* ZipArchive::getEntry(uint16_t index) const {
  if (index >= entry_count_) return nullptr;
  return &entries_[index];
}

const ZipEntry* ZipArchive::findEntry(const char* name) const {
  if (!name || !entries_) return nullptr;
  size_t target_len = strLen(name);

  for (uint16_t i = 0; i < entry_count_; ++i) {
    if (entries_[i].filename_length == target_len &&
        strEqualsLen(entries_[i].filename, name, target_len)) {
      return &entries_[i];
    }
  }
  return nullptr;
}

bool ZipArchive::extractEntry(system::System* sys, const ZipEntry* entry,
                             const uint8_t** out_data, size_t* out_size,
                             bool* out_allocated) const {
  if (!entry || !entry->data || !out_data || !out_size) return false;
  if (out_allocated) *out_allocated = false;

  // Method 0: Stored (uncompressed)
  if (entry->compression_method == 0) {
    *out_data = entry->data;
    *out_size = entry->uncompressed_size;
    return true;
  }

  // Method 8: Deflate / Inflate decompression
  if (entry->compression_method == 8) {
    if (!sys || entry->uncompressed_size == 0) return false;
    uint8_t* decomp = static_cast<uint8_t*>(sys->allocate(entry->uncompressed_size));
    if (!decomp) return false;

    if (inflateData(entry->data, entry->compressed_size, decomp, entry->uncompressed_size)) {
      *out_data = decomp;
      *out_size = entry->uncompressed_size;
      if (out_allocated) *out_allocated = true;
      return true;
    } else {
      sys->free(decomp);
      return false;
    }
  }

  return false;
}

bool ZipArchive::getMainClass(char* out_buffer, size_t max_len) const {
  if (!out_buffer || max_len == 0) return false;

  const ZipEntry* manifest = findEntry("META-INF/MANIFEST.MF");
  if (!manifest) {
    manifest = findEntry("meta-inf/manifest.mf");
  }
  if (!manifest || !manifest->data) return false;

  const char* text = nullptr;
  size_t len = 0;

  // Extract manifest data (supporting both stored and compressed manifests)
  if (manifest->compression_method == 0) {
    text = reinterpret_cast<const char*>(manifest->data);
    len = manifest->uncompressed_size;
  } else if (manifest->compression_method == 8) {
    // For manifest in memory without system pointer, allocate on stack if small
    if (manifest->uncompressed_size <= 4096) {
      uint8_t stack_buf[4096];
      if (inflateData(manifest->data, manifest->compressed_size, stack_buf, manifest->uncompressed_size)) {
        // Search inside stack buffer
        const char* key = "Main-Class:";
        size_t key_len = 11;
        const char* stext = reinterpret_cast<const char*>(stack_buf);
        size_t slen = manifest->uncompressed_size;
        for (size_t i = 0; i + key_len < slen; ++i) {
          bool match = true;
          for (size_t k = 0; k < key_len; ++k) {
            char c1 = stext[i + k];
            char c2 = key[k];
            if (c1 >= 'A' && c1 <= 'Z') c1 += ('a' - 'A');
            if (c2 >= 'A' && c2 <= 'Z') c2 += ('a' - 'A');
            if (c1 != c2) { match = false; break; }
          }
          if (match) {
            size_t pos = i + key_len;
            while (pos < slen && (stext[pos] == ' ' || stext[pos] == '\t')) ++pos;
            size_t out_idx = 0;
            while (pos < slen && stext[pos] != '\r' && stext[pos] != '\n' && stext[pos] != ' ' && out_idx + 1 < max_len) {
              out_buffer[out_idx++] = stext[pos++];
            }
            out_buffer[out_idx] = '\0';
            return out_idx > 0;
          }
        }
      }
      return false;
    }
  }

  if (!text) return false;

  // Search for "Main-Class:" (case-insensitive key match)
  const char* key = "Main-Class:";
  size_t key_len = 11;

  for (size_t i = 0; i + key_len < len; ++i) {
    bool match = true;
    for (size_t k = 0; k < key_len; ++k) {
      char c1 = text[i + k];
      char c2 = key[k];
      if (c1 >= 'A' && c1 <= 'Z') c1 += ('a' - 'A');
      if (c2 >= 'A' && c2 <= 'Z') c2 += ('a' - 'A');
      if (c1 != c2) {
        match = false;
        break;
      }
    }

    if (match) {
      size_t pos = i + key_len;
      // Skip whitespace/spaces
      while (pos < len && (text[pos] == ' ' || text[pos] == '\t')) {
        ++pos;
      }
      size_t out_idx = 0;
      while (pos < len && text[pos] != '\r' && text[pos] != '\n' && text[pos] != ' ' && out_idx + 1 < max_len) {
        out_buffer[out_idx++] = text[pos++];
      }
      out_buffer[out_idx] = '\0';
      return out_idx > 0;
    }
  }

  return false;
}

} // namespace zip
} // namespace avian
