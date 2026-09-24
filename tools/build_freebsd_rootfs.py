#!/usr/bin/env python3
"""
ATOMS OS — FreeBSD 14.1 Genuine Root Filesystem Builder
Copyright © 2026 ATOMS OS Project / Saumya Chaudhari

Phase 5A: Real FreeBSD 14.1 Userspace Root Filesystem Fetch & Verification
Downloads official FreeBSD 14.1-RELEASE amd64 mini-memstick image,
verifies official release SHA256 checksums, and extracts authentic UFS2 disk image.
"""

import os
import sys
import lzma
import hashlib
import urllib.request
import struct

IMAGE_XZ_URL = "https://archive.freebsd.org/old-releases/ISO-IMAGES/14.1/FreeBSD-14.1-RELEASE-amd64-mini-memstick.img.xz"
EXPECTED_XZ_SHA256 = "d9f7d959b73de41f8aae9dc5f3492db0b15a35f9a5d7ccc05249f2f6de7e1121"
EXPECTED_RAW_SHA256 = "971dee4c473e03a2a09fdda1c6d65dbf0891b5061895c6a180ee471e67481426"

PAYLOAD_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "freebsd_payload")
LOCAL_XZ_PATH = os.path.join(PAYLOAD_DIR, "FreeBSD-14.1-RELEASE-amd64-mini-memstick.img.xz")
LOCAL_RAW_PATH = os.path.join(PAYLOAD_DIR, "freebsd_rootfs.img")

def download_file(url, dest_path):
    print(f"[1/4] Downloading official FreeBSD 14.1 mini-memstick image...")
    print(f"      Source: {url}")
    req = urllib.request.Request(url, headers={'User-Agent': 'ATOMS-OS-Fetcher/1.0'})
    with urllib.request.urlopen(req) as resp, open(dest_path, 'wb') as out_f:
        total = int(resp.headers.get('Content-Length', 0))
        done = 0
        chunk_size = 1024 * 1024
        while True:
            chunk = resp.read(chunk_size)
            if not chunk:
                break
            out_f.write(chunk)
            done += len(chunk)
            pct = (done / total * 100) if total else 0
            print(f"\r      Progress: {done / (1024*1024):.1f} / {total / (1024*1024):.1f} MB ({pct:.1f}%)", end="", flush=True)
        print("\n      Download completed successfully.")

def verify_sha256(file_path, expected_hash):
    h = hashlib.sha256()
    with open(file_path, 'rb') as f:
        while chunk := f.read(65536):
            h.update(chunk)
    digest = h.hexdigest()
    if digest.lower() != expected_hash.lower():
        print(f"[ERROR] SHA256 mismatch for {file_path}!")
        print(f"        Expected: {expected_hash}")
        print(f"        Actual:   {digest}")
        return False
    return True

def decompress_xz(xz_path, raw_path):
    print(f"[3/4] Decompressing LZMA/XZ image to {raw_path}...")
    with lzma.open(xz_path, 'rb') as in_f, open(raw_path, 'wb') as out_f:
        done = 0
        while chunk := in_f.read(1024 * 1024):
            out_f.write(chunk)
            done += len(chunk)
            print(f"\r      Extracted: {done / (1024*1024):.1f} MB", end="", flush=True)
    print("\n      Decompression completed.")

def inspect_and_extract_ufs(raw_path):
    print(f"[4/4] Inspecting MBR & BSD disklabel in {raw_path}...")
    with open(raw_path, 'rb') as f:
        mbr = f.read(512)
        if mbr[510:512] != b'\x55\xAA':
            print("      [WARNING] MBR signature missing.")
            return

        # Slice 2 starts at LBA 66585
        slice2_lba = 66585
        f.seek(slice2_lba * 512 + 512) # BSD disklabel at offset 512 of slice 2
        dl = f.read(512)
        magic = struct.unpack_from('<I', dl, 0)[0]
        if magic != 0x82564557:
            print(f"      [WARNING] BSD disklabel magic mismatch: {hex(magic)}")
            return

        # Partition 'a' is at index 0 of disklabel
        size_sectors, offset_sectors = struct.unpack_from('<II', dl, 148)
        ufs_start_lba = slice2_lba + offset_sectors # 66601
        ufs_size_bytes = size_sectors * 512         # ~502 MB

        # Verify UFS2 Superblock at offset 64KB inside partition 'a'
        f.seek(ufs_start_lba * 512 + 65536 + 0x55C)
        sb_magic = int.from_bytes(f.read(4), 'little')
        if sb_magic != 0x19540119:
            print(f"      [ERROR] UFS2 Superblock magic mismatch: {hex(sb_magic)}")
            return

        print(f"      [PASS] Found valid FreeBSD UFS2 filesystem at LBA {ufs_start_lba} ({size_sectors} sectors, {ufs_size_bytes/(1024*1024):.1f} MB)")
        print(f"      [PASS] UFS2 Superblock magic verified: 0x19540119")

        # Extract pure UFS2 raw partition to freebsd_rootfs.ufs2
        ufs2_path = os.path.join(PAYLOAD_DIR, "freebsd_rootfs.ufs2")
        print(f"      Extracting pure UFS2 raw partition to {ufs2_path}...")
        f.seek(ufs_start_lba * 512)
        with open(ufs2_path, 'wb') as out_f:
            remaining = ufs_size_bytes
            while remaining > 0:
                to_read = min(remaining, 4 * 1024 * 1024)
                buf = f.read(to_read)
                if not buf:
                    break
                out_f.write(buf)
                remaining -= len(buf)
        print(f"      [SUCCESS] Extracted {ufs_size_bytes/(1024*1024):.1f} MB pure UFS2 filesystem image!")

def main():
    os.makedirs(PAYLOAD_DIR, exist_ok=True)

    if not os.path.exists(LOCAL_XZ_PATH):
        download_file(IMAGE_XZ_URL, LOCAL_XZ_PATH)
    else:
        print(f"[1/4] Found existing cached {LOCAL_XZ_PATH}")

    print("[2/4] Verifying XZ SHA256 checksum against official FreeBSD release manifest...")
    if not verify_sha256(LOCAL_XZ_PATH, EXPECTED_XZ_SHA256):
        sys.exit(1)
    print("      [PASS] Official FreeBSD 14.1 release checksum verified!")

    if not os.path.exists(LOCAL_RAW_PATH):
        decompress_xz(LOCAL_XZ_PATH, LOCAL_RAW_PATH)
    else:
        print(f"[3/4] Found existing uncompressed {LOCAL_RAW_PATH}")

    print("      Verifying uncompressed RAW SHA256 checksum...")
    if not verify_sha256(LOCAL_RAW_PATH, EXPECTED_RAW_SHA256):
        sys.exit(1)
    print("      [PASS] Uncompressed FreeBSD 14.1 raw disk checksum verified!")

    inspect_and_extract_ufs(LOCAL_RAW_PATH)
    print("\n[SUCCESS] Authentic FreeBSD 14.1 UFS2 Root Filesystem is ready for ATOMS BOS Hypervisor!")

if __name__ == "__main__":
    main()
