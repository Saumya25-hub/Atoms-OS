# RELEASE NOTES — ATOMS Kernel v0.9.8

```text
ATOMS Kernel v0.9.8
The Final Milestone Before Kernel v1.0
```

## Title: ATOMS Kernel v0.9.8 - Production NTFS Read-Only Certification

### Description
ATOMS Kernel v0.9.8 represents the final stabilization milestone before the upcoming v1.0 release.

This release completes the NTFS Read-Only Production Certification project and validates the filesystem driver against a genuine Microsoft Windows XP created NTFS volume.

The driver is no longer verified only with synthetic structures.

It has now been validated using real on-disk metadata, directories, files, resident data, non-resident data and fragmented runlists created by Windows XP itself.

---

### Windows XP Real-Media Certification

#### Real NTFS Validation
- ✔ Windows XP formatted NTFS partition
- ✔ Real Boot Sector
- ✔ Real MFT
- ✔ Update Sequence Arrays
- ✔ Resident Files
- ✔ Non-Resident Files
- ✔ Long Filenames
- ✔ Deep Directory Traversal
- ✔ Large Directory Enumeration
- ✔ Index Allocation
- ✔ Multi-Extent Runlists
- ✔ Fragmented Files
- ✔ Streaming Reads

---

### Verified Real Files Successfully Read

ATOMS OS successfully recovered the exact text payloads from these files on the Windows XP volume:

- `OS KERNAL.txt`
- `DEEPTEST(ATOMS).txt`
- `ATOM-HII.txt`
- `ATOM.txt`
- `ATOM_OS_READ.txt`
- `ATOM_OS_TESTS_NTFS_SYSTEM_LIKE_WINDOWSXP_TO_ATOMSOS.txt`
- `EMPTY.txt`
- `FILE1.txt`
- `FILE2.txt`
- `FILE3.txt`
- `FILE4.txt`
- `FILE5.txt`
- `FILE6.txt`
- `FILE7.txt`
- `FILE8.txt`
- `FILE9.txt`
- `FILE10.txt`
- `os details.txt`

---

### Large File Validation

- 64 KB Non-Resident File (`medium_test.bin`)
- 1 MB Streaming File (`largest_test.bin`)
- 8 MB Streaming File (`hug_test.bin`)
- 200 MB Fragmented File (`BIG1.BIN`)
- 73 Run Extents
- Real Windows XP Runlist Traversal Verified

---

### Engineering Summary

ATOMS OS now demonstrates real interoperability with a Windows XP generated NTFS filesystem.

The filesystem driver correctly parses Boot Sector structures, Master File Table records, resident and non-resident attributes, mapping pairs, fragmented runlists, long filenames and directory indexes.

Testing was performed against an actual Windows XP generated filesystem rather than synthetic metadata alone.

---

### GitHub Evidence Matrix

- **Kernel Version:** ATOMS Kernel v0.9.8
- **Status:** Production Ready (Read-Only NTFS)
- **Validation:** Real Windows XP NTFS Volume
- **Synthetic Tests:** 125 / 125 PASS
- **Real Media Tests:** PASS
- **Fragmented Runlist:** 73 Extents PASS
- **Resident Files:** PASS
- **Non-Resident Files:** PASS
- **Directory Enumeration:** PASS
- **Long Filename:** PASS
- **Deep Directory:** PASS
- **Streaming Reads:** PASS
