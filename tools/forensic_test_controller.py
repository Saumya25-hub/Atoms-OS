#!/usr/bin/env python3
"""
ATOMS OS — Automated Forensic Test Controller & Screenshot Receiver
Milestone 1: Framebuffer Capture -> BMP -> LAN UDP -> Python Receiver
"""

import sys
import os
import socket
import struct
import time
import datetime
import argparse

MAGIC_SPMS = 0x534D5053 # "SPMS" in little-endian
HEADER_FORMAT = "<IIHHIHH" # magic (I), session_id (I), total_chunks (H), chunk_index (H), offset (I), data_len (H), flags (H) = 20 bytes
HEADER_SIZE = struct.calcsize(HEADER_FORMAT)

SCREENSHOT_PORT = 9998
CONTROL_PORT = 9999
TARGET_H81_IP = "192.168.2.100"
SERVER_IP = "192.168.2.1"

OUTPUT_DIR = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "artifacts", "screenshots")

def ensure_output_dir():
    if not os.path.exists(OUTPUT_DIR):
        os.makedirs(OUTPUT_DIR, exist_ok=True)

def send_control_command(cmd, target_ip=TARGET_H81_IP, port=CONTROL_PORT):
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    msg = (cmd.strip() + "\n").encode('utf-8')
    sock.sendto(msg, (target_ip, port))
    sock.close()
    print(f"[CONTROLLER] Sent '{cmd}' to {target_ip}:{port}")

def run_screenshot_listener(timeout_sec=None, once=False):
    ensure_output_dir()
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

    try:
        sock.bind((SERVER_IP, SCREENSHOT_PORT))
    except Exception:
        sock.bind(("0.0.0.0", SCREENSHOT_PORT))

    if timeout_sec:
        sock.settimeout(timeout_sec)

    print("=" * 65)
    print("  ATOMS OS — FORENSIC SCREENSHOT RECEIVER (UDP 9998)")
    print(f"  Listening on: {sock.getsockname()[0]}:{SCREENSHOT_PORT}")
    print(f"  Screenshots Directory: {OUTPUT_DIR}")
    print("=" * 65)
    print("[RECEIVER] Waiting for ATOMS OS screenshot packets...", flush=True)

    sessions = {} # session_id -> { 'chunks': {}, 'total': 0, 'received': 0, 'start_time': 0 }

    try:
        while True:
            try:
                data, addr = sock.recvfrom(2048)
            except socket.timeout:
                print("[RECEIVER] Timeout waiting for screenshot.")
                return None

            if len(data) < HEADER_SIZE:
                continue

            header_data = data[:HEADER_SIZE]
            magic, session_id, total_chunks, chunk_index, offset, data_len, flags = struct.unpack(HEADER_FORMAT, header_data)

            if magic != MAGIC_SPMS:
                continue

            chunk_payload = data[HEADER_SIZE:HEADER_SIZE + data_len]

            if session_id not in sessions:
                sessions[session_id] = {
                    'chunks': {},
                    'total': total_chunks,
                    'received': 0,
                    'start_time': time.time()
                }
                print(f"\n[RECEIVER] Incoming screenshot stream! Session: {session_id} | Expected Chunks: {total_chunks}", flush=True)

            sess = sessions[session_id]
            if chunk_index not in sess['chunks']:
                sess['chunks'][chunk_index] = (offset, chunk_payload)
                sess['received'] += 1

                if sess['received'] % 200 == 0 or sess['received'] == sess['total']:
                    pct = (sess['received'] * 100) // sess['total']
                    print(f"[RECEIVER] Progress: {sess['received']}/{sess['total']} chunks ({pct}%)", end='\r', flush=True)

            # Check if all chunks received or end flag reached with enough chunks
            if sess['received'] >= sess['total'] or ((flags & 2) and sess['received'] >= (sess['total'] - 5)):
                duration = time.time() - sess['start_time']
                print(f"\n[RECEIVER] All {sess['received']}/{sess['total']} chunks received in {duration:.3f}s! Reassembling BMP...", flush=True)

                # Sort and assemble stream
                sorted_chunks = sorted(sess['chunks'].items(), key=lambda x: x[1][0]) # sort by offset
                bmp_bytes = bytearray()
                for c_idx, (c_offset, c_data) in sorted_chunks:
                    # Pad if needed for dropped intermediate bytes
                    if len(bmp_bytes) < c_offset:
                        bmp_bytes.extend(b'\x00' * (c_offset - len(bmp_bytes)))
                    bmp_bytes.extend(c_data)

                # Validate BMP signature
                if len(bmp_bytes) >= 54 and bmp_bytes[:2] == b'BM':
                    file_size, = struct.unpack("<I", bmp_bytes[2:6])
                    width, height, planes, bpp = struct.unpack("<iiHH", bmp_bytes[18:30])
                    timestamp_str = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
                    filename = f"screenshot_{timestamp_str}_s{session_id}.bmp"
                    filepath = os.path.join(OUTPUT_DIR, filename)

                    with open(filepath, "wb") as f:
                        f.write(bmp_bytes)

                    print("\n" + "=" * 65)
                    print("  SCREENSHOT RECEIVED")
                    print("=" * 65)
                    print(f"  File       : {filepath}")
                    print(f"  Resolution : {width}x{abs(height)} ({bpp}-bit)")
                    print(f"  File Size  : {len(bmp_bytes):,} bytes (Header: {file_size:,} bytes)")
                    print(f"  Duration   : {duration:.3f} seconds")
                    print("=" * 65 + "\n", flush=True)

                    # Try to convert to PNG as well for convenient preview
                    try:
                        from PIL import Image
                        img = Image.open(filepath)
                        png_path = filepath.rsplit('.', 1)[0] + ".png"
                        img.save(png_path)
                        print(f"  [CONVERT] Saved PNG preview: {png_path}\n", flush=True)
                    except ImportError:
                        pass

                    del sessions[session_id]

                    if once:
                        return filepath
                else:
                    print(f"[RECEIVER] Warning: Corrupted BMP magic (got {bmp_bytes[:2]}), size={len(bmp_bytes)}")
                    del sessions[session_id]

    except KeyboardInterrupt:
        print("\n[RECEIVER] Stopped by user.")
    finally:
        sock.close()

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="ATOMS OS Automated Forensic Test Controller")
    parser.add_argument("--listen", action="store_true", help="Run screenshot receiver on UDP 9998")
    parser.add_argument("--screenshot", action="store_true", help="Send remote SCREENSHOT command to ATOMS")
    parser.add_argument("--reboot", action="store_true", help="Send remote REBOOT command to ATOMS")
    parser.add_argument("--shutdown", action="store_true", help="Send remote SHUTDOWN command to ATOMS")
    args = parser.parse_args()

    if args.screenshot:
        send_control_command("SCREENSHOT")
    elif args.reboot:
        send_control_command("REBOOT")
    elif args.shutdown:
        send_control_command("SHUTDOWN")
    else:
        run_screenshot_listener()
