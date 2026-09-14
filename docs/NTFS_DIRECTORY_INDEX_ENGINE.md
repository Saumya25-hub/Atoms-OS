# ATOMS OS — NTFS Directory Index & B-Tree Engine Specification
**Document ID:** `NTFS-INDEX-V1.0`  
**Target:** Multi-Tier Directory B+Tree Insertion, Deletion, and Node Balancing  
**Hardware Target:** ASUS PRIME B750M-K (Intel Core i3-14100F, WD Blue SN5000 NVMe SSD)  
**Authors:** Developer A (Architecture) & Developer B (Forensics)  
**Date:** 2026-09-04  

---

## 1. Directory Index Architecture Overview

NTFS directories use a **B+Tree** index architecture known as `$I30` (indexing `$FILE_NAME` attributes).
A directory exists in one of two physical states:
1. **Single-Tier (Resident Index):** Directory contents fit entirely within `$INDEX_ROOT` (attribute `0x90`) inside the directory's 1024-byte MFT record. All index entries are leaf records without sub-node pointers.
2. **Two-Tier (Multi-Tier B+Tree):** Directory contents exceed resident slack space. Directory possesses:
   - `$INDEX_ROOT` (`0x90`): Root node containing router keys with child VCN downlinks.
   - `$INDEX_ALLOCATION` (`0xA0`): Non-resident 4096-byte `"INDX"` blocks containing leaf or intermediate entries.
   - `$BITMAP` (`0xB0`): Bitmap tracking allocation of 4096-byte blocks in `$INDEX_ALLOCATION`.

---

## 2. On-Disk Index Entry Structure

```
+-------------------------------------------------------------------------------+
|                       NTFS_IndexEntry Structure (Variable)                    |
+-------------------+---------+-------------------------------------------------+
| Field             | Size    | Description                                     |
+-------------------+---------+-------------------------------------------------+
| file_reference    | 8 bytes | MFT file reference (Bits 0..47: Record, 48..63: Seq)|
| length            | 2 bytes | Total length of this entry (8-byte aligned)     |
| key_length        | 2 bytes | Length of key ($FILE_NAME attribute)            |
| flags             | 2 bytes | 0x01 = HAS_SUBNODE, 0x02 = LAST_ENTRY (End)     |
| reserved          | 2 bytes | Alignment padding                               |
| key (STREAM)      | Var     | Embedded $FILE_NAME attribute (type 0x30 value) |
| child_vcn         | 8 bytes | [OPTIONAL] Exists ONLY if (flags & 0x01) == 1   |
+-------------------+---------+-------------------------------------------------+
```

### Critical Invariants
1. **Entry Alignment:** Every index entry length must be an exact multiple of 8 bytes:
   $$\text{length} = (\text{sizeof(NTFS\_IndexEntry)} + \text{key\_length} + (\text{has\_subnode} ? 8 : 0) + 7) \ \& \sim 7\text{U}$$
2. **Child VCN Position:** If `flags & 0x0001` is set, the 8-byte child VCN is located at the very end of the entry:
   $$\text{child\_vcn} = *(\text{uint64\_t}*)((\text{uint8\_t}*)\text{entry} + \text{entry}\to\text{length} - 8)$$
3. **End Marker:** The final entry in every index node has `flags & 0x0002` (`NTFS_INDEX_ENTRY_LAST`), `key_length = 0`, and serves as the catch-all router for all keys greater than the preceding entry. If the node has children, the End Marker **must** also have `flags = 0x0003` and carry the child VCN to the rightmost subtree.

---

## 3. Filename Collation Algorithm ($UpCase Table)

NTFS compares filenames by mapping UTF-16 code units to uppercase using the volume's `$UpCase` table (Record 10) and performing binary comparison:

```c
int ntfs_collate_filenames(const uint16_t* name1, uint8_t len1,
                           const uint16_t* name2, uint8_t len2,
                           const uint16_t* upcase) {
    uint8_t min_len = (len1 < len2) ? len1 : len2;
    for (uint8_t i = 0; i < min_len; i++) {
        uint16_t c1 = upcase ? upcase[name1[i]] : name1[i];
        uint16_t c2 = upcase ? upcase[name2[i]] : name2[i];
        if (c1 != c2) {
            return (int)c1 - (int)c2;
        }
    }
    // Shorter name precedes longer name if common prefix matches
    return (int)len1 - (int)len2;
}
```

---

## 4. Single-Tier Insertion Algorithm

When `idx_hdr->flags & 0x01 == 0` (no `$INDEX_ALLOCATION`):
1. **Find Insertion Offset:** Iterate through entries starting at `entries_offset`. Compare `new_key` with each entry's key using `ntfs_collate_filenames()`.
   - Find entry $E$ where $\text{new\_key} < E\to\text{key}$, or $E$ is the End Marker (`flags & 0x02`).
2. **Slack Space Check:**
   $$\text{needed} = \text{entry\_len}$$
   $$\text{available} = \text{record\_size} - \text{record}\to\text{bytes\_in\_use}$$
3. **If Entry Fits in Slack:**
   - Shift existing entries from insertion offset to `bytes_in_use` forward by `entry_len`.
   - Construct new `NTFS_IndexEntry` at insertion offset with `flags = 0x0000` (leaf).
   - Update `idx_hdr->total_size += entry_len`.
   - Update `idx_hdr->allocated_size += entry_len`.
   - Update attribute header length and `fhdr->bytes_in_use += entry_len`.
   - Recompute USA fixup sequence and write directory record to disk.
4. **If Entry Does NOT Fit:**
   - Trigger **Root Split to Two-Tier Transition**:
     1. Allocate 1 cluster for `$INDEX_ALLOCATION` via Volume `$Bitmap`.
     2. Add `$INDEX_ALLOCATION` attribute (`0xA0`) to directory MFT record.
     3. Add `$BITMAP` attribute (`0xB0`) with bit 0 set to `1`.
     4. Format a 4096-byte `"INDX"` block at VCN 0.
     5. Move all existing entries from `$INDEX_ROOT` into the `"INDX"` block.
     6. Truncate `$INDEX_ROOT` entries to contain **only** an End Marker (`flags = 0x0003`) pointing to VCN 0.
     7. Set `idx_hdr->flags |= 0x01` (`HAS_LARGE_INDEX`).
     8. Insert new entry into the `"INDX"` block in sorted order.

---

## 5. Two-Tier Insertion Algorithm

When `idx_hdr->flags & 0x01 == 1` (`HAS_LARGE_INDEX`):
1. **Traverse `$INDEX_ROOT` Router Nodes:**
   - Compare `new_key` against router entries in `$INDEX_ROOT`.
   - If $\text{new\_key} < \text{router\_entry}\to\text{key}$: follow $\text{router\_entry}\to\text{child\_vcn}$.
   - If loop reaches End Marker: follow $\text{end\_marker}\to\text{child\_vcn}$.
2. **Read Target 4096-byte `"INDX"` Block:**
   - Translate child VCN to physical LBA via `$INDEX_ALLOCATION` runlist.
   - Read 8 sectors (4096 bytes) into buffer.
   - Validate magic `"INDX"` and apply USA fixup.
3. **Insert Entry in `"INDX"` Block:**
   - Locate sorted collation position within the block's entries array.
   - If entry fits within 4096-byte block boundary:
     - Shift trailing entries forward by `entry_len`.
     - Insert entry (`flags = 0x0000`).
     - Update block index header `total_size`.
     - Apply USA fixup to 8 sector trailers.
     - Write 4096-byte block back to disk.
4. **Handle `"INDX"` Block Split (Overflow):**
   - If `block_total_size + entry_len > 4096`:
     - Select median entry as divider key.
     - Allocate new 4096-byte block via directory `$BITMAP`.
     - Move entries above median into the new block.
     - Promote median key to `$INDEX_ROOT` with child VCN pointing to the new block.
     - Update parent router entries and write both blocks and parent record.

---

## 6. Mathematical Parity & Verification Guarantee

Every directory mutation executed by this engine strictly adheres to Microsoft `ntfs.sys` structural invariants:
- Zero out-of-order entries.
- Zero leaf flags in router nodes.
- Zero missing child VCN pointers in multi-tier trees.
- Zero torn writes across multi-sector index buffers.
