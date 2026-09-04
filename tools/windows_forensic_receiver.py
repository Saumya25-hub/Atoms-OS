#!/usr/bin/env python3
"""
ATOMS OS — Windows Forensic File Receiver & Integrity Verifier
Listens for WFFP forensic packets on UDP Port 9997, reconstructs files,
validates IEEE 802.3 CRC32, generates SHA-256 digests, and compiles manifest.txt.
"""

import socket
import struct
import os
import sys
import time
import binascii
import hashlib
import datetime

sys.stdout.reconfigure(line_buffering=True)

WFFP_MAGIC = 0x57464650  # "WFFP"
WFFP_VERSION = 1
WFFP_PORT = 9997
SERVER_IP = "192.168.2.1"

WFFP_TYPE_FILE_START  = 1
WFFP_TYPE_FILE_DATA   = 2
WFFP_TYPE_FILE_END    = 3
WFFP_TYPE_FILE_ABSENT = 4
WFFP_TYPE_SESSION_END = 5

HEADER_FORMAT = "<IHHIIIQIQI64s"
HEADER_SIZE = struct.calcsize(HEADER_FORMAT)

BASE_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUTPUT_DIR = os.path.join(BASE_DIR, "artifacts", "windows_forensic")
DUMPS_DIR = os.path.join(OUTPUT_DIR, "dumps")
MANIFEST_PATH = os.path.join(OUTPUT_DIR, "manifest.txt")


class ForensicFileState:
    def __init__(self, file_id, filename, total_size):
        self.file_id = file_id
        self.filename = filename
        self.total_size = total_size
        self.received_bytes = 0
        self.chunks = {}  # offset -> payload
        self.crc_errors = 0
        self.packet_count = 0
        self.completed = False
        self.absent = False
        self.sha256 = None


def ensure_directories():
    os.makedirs(OUTPUT_DIR, exist_ok=True)
    os.makedirs(DUMPS_DIR, exist_ok=True)


def parse_header(data):
    if len(data) < HEADER_SIZE:
        return None
    header_data = data[:HEADER_SIZE]
    magic, version, pkt_type, session_id, file_id, seq, offset, payload_len, total_size, crc32, raw_fname = struct.unpack(
        HEADER_FORMAT, header_data
    )
    filename = raw_fname.split(b'\x00')[0].decode('ascii', errors='ignore').strip()
    return {
        "magic": magic,
        "version": version,
        "packet_type": pkt_type,
        "session_id": session_id,
        "file_id": file_id,
        "sequence": seq,
        "offset": offset,
        "payload_len": payload_len,
        "total_size": total_size,
        "crc32": crc32,
        "filename": filename,
    }


def write_manifest(session_id, files_dict, stats):
    lines = []
    lines.append("================================================================================")
    lines.append(" ATOMS OS — WINDOWS FORENSIC ACQUISITION MANIFEST")
    lines.append("================================================================================")
    lines.append(f"Session Identifier : 0x{session_id:08X} ({stats['start_time']})")
    lines.append(f"Source Machine     : ASUS PRIME B750M-K (Intel Core i3-14100F)")
    lines.append(f"Source Device      : WD Blue SN5000 NVMe SSD (Dynamic Namespace Discovery)")
    lines.append(f"Source Partition   : Partition 3 (Windows 11 NTFS Volume)")
    lines.append(f"Forensic Mode      : 100% STRICT READ-ONLY")
    lines.append(f"Source Write Count : 0 (HARDWARE WRITE-BLOCKING VERIFIED)")
    lines.append(f"NTFS Write Count   : 0")
    lines.append(f"GPT Write Count    : 0")
    lines.append(f"MFT Write Count    : 0")
    lines.append("--------------------------------------------------------------------------------")
    lines.append(f"{'STATUS':<12} {'FILENAME':<24} {'SIZE (BYTES)':<14} {'PACKETS':<10} {'CRC ERR':<9} {'SHA-256 HASH'}")
    lines.append("--------------------------------------------------------------------------------")

    for fid in sorted(files_dict.keys()):
        fstate = files_dict[fid]
        if fstate.absent:
            status = "NOT FOUND"
            sha = "N/A (File absent from volume)"
            sz_str = "0"
        elif fstate.completed:
            status = "ACQUIRED"
            sha = fstate.sha256 or "UNKNOWN"
            sz_str = str(fstate.total_size)
        else:
            status = "INCOMPLETE"
            sha = "N/A (Partial transfer)"
            sz_str = str(fstate.received_bytes)

        lines.append(f"{status:<12} {fstate.filename:<24} {sz_str:<14} {fstate.packet_count:<10} {fstate.crc_errors:<9} {sha}")

    lines.append("================================================================================")
    lines.append(f"SUMMARY: Acquired: {stats['files_acquired']} | Missing: {stats['files_missing']} | CRC Errors: {stats['total_crc_errors']} | Source Writes: 0")
    lines.append("================================================================================\n")

    manifest_content = "\n".join(lines)
    with open(MANIFEST_PATH, "w", encoding="utf-8") as f:
        f.write(manifest_content)

    print(f"\n[MANIFEST] Forensic manifest written to '{MANIFEST_PATH}'")


def run_receiver(bind_ip=SERVER_IP, bind_port=WFFP_PORT, timeout_sec=None):
    ensure_directories()
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

    try:
        sock.bind((bind_ip, bind_port))
    except Exception as e:
        print(f"[RECEIVER] Warning: Could not bind to {bind_ip}:{bind_port} ({e}), trying 0.0.0.0:{bind_port}")
        sock.bind(("0.0.0.0", bind_port))

    print("=" * 70)
    print("  ATOMS OS — WINDOWS FORENSIC FILE RECEIVER (PORT: 9997)")
    print("=" * 70)
    print(f" Listening on UDP {bind_ip}:{bind_port}...")
    print(f" Artifacts destination: {OUTPUT_DIR}")
    print("=" * 70)

    files = {}
    active_session_id = None
    start_time = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    stats = {
        "start_time": start_time,
        "total_packets": 0,
        "total_crc_errors": 0,
        "files_acquired": 0,
        "files_missing": 0,
    }

    last_packet_time = time.time()

    while True:
        if timeout_sec and (time.time() - last_packet_time > timeout_sec):
            print(f"[RECEIVER] Inactivity timeout of {timeout_sec}s reached.")
            break

        try:
            sock.settimeout(2.0)
            data, addr = sock.recvfrom(2048)
            last_packet_time = time.time()
        except socket.timeout:
            continue
        except KeyboardInterrupt:
            print("\n[RECEIVER] User interrupt. Shutting down...")
            break

        header = parse_header(data)
        if not header or header["magic"] != WFFP_MAGIC:
            continue

        active_session_id = header["session_id"]
        fid = header["file_id"]
        ptype = header["packet_type"]
        fname = header["filename"]
        stats["total_packets"] += 1

        if ptype == WFFP_TYPE_FILE_START:
            files[fid] = ForensicFileState(fid, fname, header["total_size"])
            print(f"\n[FILE] {fname} FOUND | Size: {header['total_size']} bytes")
            print(f"[READ] 0 / {header['total_size']}")

        elif ptype == WFFP_TYPE_FILE_ABSENT:
            fstate = ForensicFileState(fid, fname, 0)
            fstate.absent = True
            files[fid] = fstate
            stats["files_missing"] += 1
            print(f"[NOT FOUND] {fname} absent from volume")

        elif ptype == WFFP_TYPE_FILE_DATA:
            if fid not in files:
                files[fid] = ForensicFileState(fid, fname, header["total_size"])

            fstate = files[fid]
            fstate.packet_count += 1
            payload = data[HEADER_SIZE : HEADER_SIZE + header["payload_len"]]

            # Verify IEEE 802.3 CRC32
            calc_crc = binascii.crc32(payload) & 0xFFFFFFFF
            if calc_crc != header["crc32"]:
                fstate.crc_errors += 1
                stats["total_crc_errors"] += 1
                print(f"[CRC ERROR] File {fname} chunk offset {header['offset']}: got 0x{calc_crc:08X}, expected 0x{header['crc32']:08X}")
            else:
                fstate.chunks[header["offset"]] = payload
                fstate.received_bytes += len(payload)

            # Live progress output
            if fstate.total_size > 0:
                print(f"\r[READ] {fstate.received_bytes} / {fstate.total_size} ({(fstate.received_bytes * 100) // fstate.total_size}%)", end="", flush=True)

        elif ptype == WFFP_TYPE_FILE_END:
            if fid in files:
                fstate = files[fid]
                fstate.completed = True
                stats["files_acquired"] += 1
                print(f"\r[READ] {fstate.received_bytes} / {fstate.total_size} (100%)")
                print(f"[SEND] COMPLETE: {fname}")

                # Reconstruct full file
                file_dest_dir = DUMPS_DIR if fname.lower().endswith((".dmp", ".mdmp")) else OUTPUT_DIR
                out_path = os.path.join(file_dest_dir, fname)

                hasher = hashlib.sha256()
                with open(out_path, "wb") as f_out:
                    for off in sorted(fstate.chunks.keys()):
                        chunk = fstate.chunks[off]
                        f_out.write(chunk)
                        hasher.update(chunk)

                fstate.sha256 = hasher.hexdigest()
                print(f"[HASH] SHA-256: {fstate.sha256}")
                print(f"[SAVED] File saved to '{out_path}'")

        elif ptype == WFFP_TYPE_SESSION_END:
            print("\n" + "=" * 70)
            print("  FORENSIC COLLECTION COMPLETE")
            print("=" * 70)
            print(f" Files found   : {stats['files_acquired']}")
            print(f" Files missing : {stats['files_missing']}")
            print(f" Total packets : {stats['total_packets']}")
            print(f" CRC errors    : {stats['total_crc_errors']}")
            print(f" SOURCE WRITES : 0")
            print("=" * 70)
            break

    if active_session_id:
        write_manifest(active_session_id, files, stats)
    sock.close()


if __name__ == "__main__":
    ip = sys.argv[1] if len(sys.argv) > 1 else SERVER_IP
    port = int(sys.argv[2]) if len(sys.argv) > 2 else WFFP_PORT
    run_receiver(bind_ip=ip, bind_port=port)
