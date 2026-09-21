#!/usr/bin/env python3
import socket
import struct
import time
import os
from PIL import Image

SERVER_IP = "0.0.0.0"
TARGET_IP = "192.168.2.100"
LISTEN_PORT = 9998
CONTROL_PORT = 9999
MAGIC_SPMS = 0x534D5053
HEADER_FORMAT = "<IIHHIHH"
HEADER_SIZE = struct.calcsize(HEADER_FORMAT)

OUTPUT_DIR = os.path.join(os.path.dirname(__file__), "..", "artifacts", "screenshots")
os.makedirs(OUTPUT_DIR, exist_ok=True)

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
sock.bind((SERVER_IP, LISTEN_PORT))
sock.settimeout(3.0)

# Send trigger
ctrl = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
ctrl.sendto(b"SCREENSHOT\n", (TARGET_IP, CONTROL_PORT))
ctrl.close()
print(f"[CONTROLLER] Triggered SCREENSHOT to {TARGET_IP}:{CONTROL_PORT}")

sessions = {}
start_time = time.time()
captured_file = None

while time.time() - start_time < 8.0:
    try:
        data, addr = sock.recvfrom(2048)
    except socket.timeout:
        break
    except Exception as e:
        print(f"[ERROR] {e}")
        break

    if len(data) < HEADER_SIZE:
        continue

    magic, session_id, total_chunks, chunk_index, offset, data_len, flags = struct.unpack(HEADER_FORMAT, data[:HEADER_SIZE])
    if magic != MAGIC_SPMS:
        continue

    chunk_payload = data[HEADER_SIZE:HEADER_SIZE + data_len]
    if session_id not in sessions:
        sessions[session_id] = {'chunks': {}, 'total': total_chunks}
        print(f"[RECEIVER] Receiving session {session_id} ({total_chunks} chunks)...")

    sess = sessions[session_id]
    sess['chunks'][chunk_index] = (offset, chunk_payload)

    if len(sess['chunks']) >= total_chunks or (flags & 2):
        print(f"[RECEIVER] Reassembling {len(sess['chunks'])} chunks...")
        sorted_chunks = sorted(sess['chunks'].items(), key=lambda x: x[1][0])
        bmp_bytes = bytearray()
        for _, (c_off, c_data) in sorted_chunks:
            if len(bmp_bytes) < c_off:
                bmp_bytes.extend(b'\x00' * (c_off - len(bmp_bytes)))
            bmp_bytes.extend(c_data)

        if len(bmp_bytes) >= 54 and bmp_bytes[:2] == b'BM':
            ts = time.strftime("%Y%m%d_%H%M%S")
            bmp_path = os.path.join(OUTPUT_DIR, f"live_hardware_{ts}.bmp")
            png_path = os.path.join(OUTPUT_DIR, f"live_hardware_{ts}.png")
            with open(bmp_path, "wb") as f:
                f.write(bmp_bytes)
            img = Image.open(bmp_path)
            img.save(png_path)
            print(f"[SUCCESS] Saved screenshot to {png_path}")
            captured_file = png_path
            try:
                os.startfile(png_path)
                print(f"[PREVIEW] Opened photo in Windows viewer: {png_path}")
            except Exception as e:
                pass
            break

sock.close()
if not captured_file:
    print("[INFO] No new packets received within timeout (Target may be awaiting or already captured).")
