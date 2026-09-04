# ATOMS OS — NTFS Write Semantics & Data Stream Specification
**Document ID:** `NTFS-WRITE-V1.0`  
**Target:** Resident / Non-Resident File Writing, Size Semantics, and Cluster Management  
**Hardware Target:** ASUS PRIME B750M-K (Intel Core i3-14100F, WD Blue SN5000 NVMe SSD)  
**Authors:** Developer A (Architecture) & Developer B (Forensics)  
**Date:** 2026-09-04  

---

## 1. NTFS File Size Attributes

Every NTFS file tracks three distinct size metrics inside its `$DATA` attribute:
1. **Allocated Size (`allocated_size`):** Total bytes reserved on disk.
   - For resident files: Exactly matches or rounds to 8-byte alignment of value length.
   - For non-resident files: Must be an exact multiple of cluster size ($\text{clusters} \times \text{bytes\_per\_cluster}$).
2. **Real / Logical Size (`data_size`):** The actual byte count visible to applications.
3. **Valid Data Length (`initialized_size` / VDL):** The byte boundary up to which deterministic data has been written. Any read between VDL and `data_size` returns zeros.

---

## 2. Resident vs. Non-Resident Write Transitions

```
+-------------------------------------------------------------------------------+
|                            DATA WRITE ROUTING DECISION                        |
+-------------------------------------------------------------------------------+
                                       │
                    Is total file size <= 256 bytes?
                                       │
                      ┌────────────────┴────────────────┐
                     YES                               NO
                      ▼                                 ▼
             [RESIDENT STREAM]                [NON-RESIDENT STREAM]
   Stored directly in MFT record.       Allocates external clusters.
   No external clusters allocated.      Constructs data runlist.
   Zero fragmentation risk.             Updates Volume $Bitmap.
```

### 2.1 Resident Stream Writing
- **Threshold:** File payloads $\le 256$ bytes are held strictly resident inside the 1024-byte MFT record.
- **Buffer Modification:**
  - Update `res_hdr->value_length = new_size`.
  - Copy data bytes to `rec->buffer + attr_offset + res_hdr->value_offset`.
  - Update total attribute length: $\text{length} = (\text{sizeof(NTFS\_AttributeHeader)} + \text{sizeof(NTFS\_ResidentAttributeHeader)} + \text{new\_size} + 7) \ \& \sim 7\text{U}$.
  - Shift any trailing attributes (such as `$END`) and update `fhdr->bytes_in_use`.
  - Recompute USA fixup sequence and write record to disk.

### 2.2 Resident-to-Non-Resident Promotion
When an existing resident file grows beyond 256 bytes:
1. Calculate required clusters:
   $$\text{clusters} = \left\lceil \frac{\text{new\_size}}{\text{bytes\_per\_cluster}} \right\rceil$$
2. Allocate clusters via Volume `$Bitmap` (`Record 6`).
3. Write payload data to the newly allocated physical clusters.
4. Replace the resident `$DATA` attribute header with a non-resident `$DATA` attribute header (`NTFS_NonResidentAttributeHeader`).
5. Encode the allocated cluster run into compressed data runs using `ntfs_encode_data_runs()`.
6. Update `allocated_size`, `data_size`, and `initialized_size`.
7. Shift trailing attributes and update MFT record `bytes_in_use`.
8. Write modified MFT record to physical storage.

---

## 3. Directory Index Size Synchronization

Whenever a file's `$DATA` attribute size changes:
- In addition to updating the file's own MFT record, the file's entry inside its parent directory index **must also be synchronized**:
  - Locate the file's entry in the parent directory's `$INDEX_ROOT` or `$INDEX_ALLOCATION` `"INDX"` block.
  - Update `fn_attr->real_size = new_size`.
  - Update `fn_attr->allocated_size = new_allocated_size`.
  - Write updated index block to disk with proper USA fixup.
- Neglecting to synchronize the directory index entry causes a size discrepancy between directory listings and file reads.
