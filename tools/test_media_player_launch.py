#!/usr/bin/env python3
import os
import sys
import time
import socket
import json
import subprocess
from PIL import Image

LOG_FILE = r"build\qemu_media_launch.log"
PPM_FILE = r"build\media_launch_screen.ppm"
PNG_FILE = r"build\media_launch_screen.png"
QMP_PORT = 4486

for p in [LOG_FILE, PPM_FILE, PNG_FILE]:
    if os.path.exists(p):
        try: os.remove(p)
        except: pass

print("[TEST-MEDIA] Launching QEMU in UEFI mode...")
proc = subprocess.Popen([
    r"D:\OS-QEMU-EMU\qemu\qemu-system-x86_64.exe",
    "-drive", r"if=pflash,format=raw,readonly=on,file=D:\OS-QEMU-EMU\qemu\share\edk2-x86_64-code.fd",
    "-drive", r"file=build\atoms_uefi_test.img,format=raw",
    "-device", "qemu-xhci",
    "-device", "usb-kbd",
    "-device", "usb-tablet",
    "-device", "intel-hda",
    "-device", "hda-duplex",
    "-netdev", "user,id=net0",
    "-device", "e1000,netdev=net0",
    "-m", "2048M",
    "-qmp", f"tcp:127.0.0.1:{QMP_PORT},server,nowait",
    "-serial", f"file:{LOG_FILE}",
    "-display", "none"
])

print("[TEST-MEDIA] Waiting for desktop ready...")
booted = False
for i in range(40):
    time.sleep(1.0)
    if os.path.exists(LOG_FILE):
        with open(LOG_FILE, "r", errors="ignore") as f:
            content = f.read()
        if "[DESKTOP] DESKTOP RENDERED" in content or "BWE_Initialize() COMPLETE!" in content:
            print(f"[TEST-MEDIA] Desktop ready in {i+1}s!")
            booted = True
            break

if not booted:
    print("[TEST-MEDIA] Boot timeout!")
    proc.kill()
    sys.exit(1)

# Connect to QMP
s = socket.socket()
s.connect(('127.0.0.1', QMP_PORT))
s.recv(1024)
s.sendall(b'{"execute": "qmp_capabilities"}\r\n')
s.recv(1024)

def mouse_move(x, y):
    msg = json.dumps({
        "execute": "input-send-event",
        "arguments": {
            "events": [
                {"type": "abs", "data": {"axis": "x", "value": int(x * 32767 / 2560)}},
                {"type": "abs", "data": {"axis": "y", "value": int(y * 32767 / 1600)}}
            ]
        }
    }) + "\r\n"
    s.sendall(msg.encode())
    s.recv(1024)
    time.sleep(0.1)

def mouse_click(x, y):
    mouse_move(x, y)
    time.sleep(0.1)
    msg_down = json.dumps({
        "execute": "input-send-event",
        "arguments": {"events": [{"type": "btn", "data": {"button": "left", "down": True}}]}
    }) + "\r\n"
    s.sendall(msg_down.encode())
    s.recv(1024)
    time.sleep(0.1)
    msg_up = json.dumps({
        "execute": "input-send-event",
        "arguments": {"events": [{"type": "btn", "data": {"button": "left", "down": False}}]}
    }) + "\r\n"
    s.sendall(msg_up.encode())
    s.recv(1024)
    time.sleep(0.2)

def double_click(x, y):
    mouse_click(x, y)
    time.sleep(0.15)
    mouse_click(x, y)
    time.sleep(0.3)

time.sleep(2.0)

# Step 1: Click Files Icon at (62, 152)
print("[TEST-MEDIA] Opening Files (File Explorer)...")
double_click(62, 152)
time.sleep(2.5)

# Step 2: In File Explorer (wx = (2560 - 660)/2 = 950, wy = (1600 - 440)/2 = 580)
# USB tab is at: wx + 50, wy + 100 -> (1000, 680)
print("[TEST-MEDIA] Selecting USB Drive tab...")
mouse_click(1000, 680)
time.sleep(2.0)

# Step 3: Row 0 (TEST.MP4) is at cx = wx + 160 = 1110, cy = wy + 87 = 667
print("[TEST-MEDIA] Clicking Row 0 (TEST.MP4)...")
mouse_click(1150, 667)
time.sleep(1.0)
mouse_click(1150, 667)

# Monitor for playback
print("[TEST-MEDIA] Monitoring for Media Player playback & frame presentation...")
frames_presented = False
for i in range(30):
    time.sleep(1.0)
    if os.path.exists(LOG_FILE):
        with open(LOG_FILE, "r", errors="ignore") as f:
            content = f.read()
        if "FRAME_PRESENT:" in content or "PRESENTED=" in content:
            print(f"[TEST-MEDIA] Frames presenting detected in {i+1}s!")
            frames_presented = True
            break
        if "FRAME_DECODED:" in content:
            print(f"[TEST-MEDIA] Frames decoding detected in {i+1}s!")

time.sleep(3.0)

# Screendump
print("[TEST-MEDIA] Capturing screen dump...")
s.sendall(json.dumps({
    "execute": "screendump",
    "arguments": {"filename": PPM_FILE.replace("\\", "/")}
}).encode() + b"\r\n")
s.recv(1024)
time.sleep(1.0)

s.close()
proc.kill()
try: proc.wait(timeout=3)
except: pass

if os.path.exists(PPM_FILE):
    try:
        img = Image.open(PPM_FILE)
        img.save(PNG_FILE)
        print(f"[TEST-MEDIA] Saved screenshot: {PNG_FILE} ({img.width}x{img.height})")
    except Exception as ex:
        print(f"[TEST-MEDIA] Image conversion error: {ex}")

if os.path.exists(LOG_FILE):
    with open(LOG_FILE, "r", errors="ignore") as f:
        log = f.read()
    print("\n--- TEST SUMMARY ---")
    print("Media Player Launched:", "PROCESS_CREATE: media_player.elf" in log)
    print("Video Decoded:", "FRAME_DECODED:" in log)
    print("Frames Presented:", "FRAME_PRESENT:" in log)
    print("Sync Telemetry:", "PRESENTED=" in log)
    print("Kernel Panics:", "Kernel Panic" in log or "CRITICAL PANIC" in log)
