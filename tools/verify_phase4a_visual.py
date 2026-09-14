import time
import subprocess
import socket
import json
import os
from PIL import Image

LOG_FILE = r"build\qmp_phase4a_test.log"
PPM_FILE_MAIN = r"build\phase4a_main.ppm"
PPM_FILE_DIAG = r"build\phase4a_diag.ppm"
ARTIFACT_DIR = r"C:\Users\Saumya Chaudhari\.gemini\antigravity-ide\brain\09feded4-1087-4ee2-a7c5-fcd836a51008"

for p in [LOG_FILE, PPM_FILE_MAIN, PPM_FILE_DIAG]:
    if os.path.exists(p):
        try: os.remove(p)
        except: pass

print("[QMP-4A] Launching QEMU in pure UEFI mode...")
proc = subprocess.Popen([
    r"D:\OS-QEMU-EMU\qemu\qemu-system-x86_64.exe",
    "-drive", r"if=pflash,format=raw,readonly=on,file=D:\OS-QEMU-EMU\qemu\share\edk2-x86_64-code.fd",
    "-drive", r"file=build\atoms_uefi_test.img,format=raw",
    "-device", "qemu-xhci",
    "-device", "usb-kbd",
    "-device", "usb-tablet",
    "-netdev", "user,id=net0",
    "-device", "e1000,netdev=net0",
    "-m", "2048M",
    "-qmp", "tcp:127.0.0.1:4460,server,nowait",
    "-serial", f"file:{LOG_FILE}",
    "-display", "none"
])

print("[QMP-4A] Waiting for Login Supervisor Loop in serial log...")
reached_login = False
for i in range(40):
    time.sleep(1)
    if os.path.exists(LOG_FILE):
        with open(LOG_FILE, "r", errors="ignore") as f:
            content = f.read()
            if "Entering Interactive Login Supervisor Loop" in content:
                print(f"[QMP-4A] Reached login loop in {i+1}s!")
                reached_login = True
                break

if not reached_login:
    print("[QMP-4A] ERROR: Did not reach login loop within timeout!")
    proc.kill()
    exit(1)

time.sleep(1.0)
s = socket.socket()
s.connect(('127.0.0.1', 4460))
s.recv(1024)
s.sendall(b'{"execute": "qmp_capabilities"}\r\n')
s.recv(1024)

def send_qcode(k, hold=150):
    msg = json.dumps({
        "execute": "send-key",
        "arguments": {
            "keys": [{"type": "qcode", "data": k}],
            "hold-time": hold
        }
    }) + "\r\n"
    s.sendall(msg.encode())
    s.recv(1024)
    time.sleep(0.35)

def click_screen(x, y, w=2560, h=1600):
    nx = int(x * 32767 / w)
    ny = int(y * 32767 / h)
    msg_move = json.dumps({
        "execute": "input-send-event",
        "arguments": {"events": [
            {"type": "abs", "data": {"axis": "x", "value": nx}},
            {"type": "abs", "data": {"axis": "y", "value": ny}}
        ]}
    }) + "\r\n"
    s.sendall(msg_move.encode())
    s.recv(1024)
    time.sleep(0.15)

    msg_down = json.dumps({
        "execute": "input-send-event",
        "arguments": {"events": [
            {"type": "btn", "data": {"button": "left", "down": True}}
        ]}
    }) + "\r\n"
    s.sendall(msg_down.encode())
    s.recv(1024)
    time.sleep(0.15)

    msg_up = json.dumps({
        "execute": "input-send-event",
        "arguments": {"events": [
            {"type": "btn", "data": {"button": "left", "down": False}}
        ]}
    }) + "\r\n"
    s.sendall(msg_up.encode())
    s.recv(1024)
    time.sleep(0.2)

# 1. Dismiss lock screen
print("[QMP-4A] Dismissing lock screen...")
send_qcode("spc")
time.sleep(3.5)

# 2. Type password 'admin123'
print("[QMP-4A] Typing password 'admin123'...")
for key in ["a", "d", "m", "i", "n", "1", "2", "3"]:
    send_qcode(key)

time.sleep(1.0)
print("[QMP-4A] Pressing Enter...")
send_qcode("ret")

print("[QMP-4A] Waiting 12 seconds for desktop render...")
time.sleep(12.0)

# Screendump 1: Main Controls & UI tab
print("[QMP-4A] Screendump Controls tab...")
s.sendall(json.dumps({
    "execute": "screendump",
    "arguments": {"filename": PPM_FILE_MAIN.replace("\\", "/")}
}).encode() + b"\r\n")
s.recv(1024)
time.sleep(2.0)

print("[QMP-4A] Switching to Diagnostics tab via key '2'...")
send_qcode("2")
time.sleep(2.0)

# Screendump 2: Diagnostics tab
print("[QMP-4A] Screendump Diagnostics tab...")
s.sendall(json.dumps({
    "execute": "screendump",
    "arguments": {"filename": PPM_FILE_DIAG.replace("\\", "/")}
}).encode() + b"\r\n")
s.recv(1024)
time.sleep(2.0)

s.close()
proc.kill()
print("[QMP-4A] QEMU terminated cleanly.")

# Process screenshots
if os.path.exists(PPM_FILE_MAIN):
    img_main = Image.open(PPM_FILE_MAIN)
    img_main.save(os.path.join(ARTIFACT_DIR, "phase4a_desktop.png"))
    print(f"[QMP-4A] Saved full desktop capture ({img_main.width}x{img_main.height})")

    # Crop window
    box = (int(1280 - 460), int(800 - 320), int(1280 + 460), int(800 + 320))
    win_main = img_main.crop(box)
    win_main.save(os.path.join(ARTIFACT_DIR, "phase4a_window_detail.png"))
    print("[QMP-4A] Saved Controls tab window detail")

if os.path.exists(PPM_FILE_DIAG):
    img_diag = Image.open(PPM_FILE_DIAG)
    box = (int(1280 - 460), int(800 - 320), int(1280 + 460), int(800 + 320))
    win_diag = img_diag.crop(box)
    win_diag.save(os.path.join(ARTIFACT_DIR, "phase4a_diagnostics_detail.png"))
    print("[QMP-4A] Saved Diagnostics tab window detail")

# Check serial log
if os.path.exists(LOG_FILE):
    with open(LOG_FILE, "r", errors="ignore") as f:
        log_content = f.read()
    print("=== SERIAL LOG AUDIT ===")
    for test_name in ["TEST_A", "TEST_B", "TEST_C", "TEST_D", "TEST_E", "TEST_F", "TEST_G", "TEST_H", "TEST_I", "TEST_J", "TEST_K", "TEST_L"]:
        if test_name in log_content:
            print(f"  {test_name}: FOUND")
        else:
            print(f"  {test_name}: NOT FOUND in log")
    if "Kernel Panic" in log_content:
        print("  WARNING: Kernel Panic detected!")
    else:
        print("  Clean execution: Zero Kernel Panics")
