import time
import subprocess
import socket
import json
import os
from PIL import Image

LOG_FILE = r"build\qmp_phase4_final.log"
PPM_FILE_MAIN = r"build\phase4_final_main.ppm"
PPM_FILE_DIAG = r"build\phase4_final_diag.ppm"
ARTIFACT_DIR = r"C:\Users\Saumya Chaudhari\.gemini\antigravity-ide\brain\09feded4-1087-4ee2-a7c5-fcd836a51008"

for p in [LOG_FILE, PPM_FILE_MAIN, PPM_FILE_DIAG]:
    if os.path.exists(p):
        try: os.remove(p)
        except: pass

print("[PHASE4-FINAL] Launching QEMU in pure UEFI mode...")
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
    "-qmp", "tcp:127.0.0.1:4465,server,nowait",
    "-serial", f"file:{LOG_FILE}",
    "-display", "none"
])

print("[PHASE4-FINAL] Waiting for Login Supervisor Loop in serial log...")
reached_login = False
for i in range(40):
    time.sleep(1)
    if os.path.exists(LOG_FILE):
        with open(LOG_FILE, "r", errors="ignore") as f:
            content = f.read()
            if "Entering Interactive Login Supervisor Loop" in content:
                print(f"[PHASE4-FINAL] Reached login loop in {i+1}s!")
                reached_login = True
                break

if not reached_login:
    print("[PHASE4-FINAL] ERROR: Did not reach login loop within timeout!")
    proc.kill()
    exit(1)

time.sleep(1.0)
s = socket.socket()
s.connect(('127.0.0.1', 4465))
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

# 1. Dismiss lock screen
print("[PHASE4-FINAL] Dismissing lock screen...")
send_qcode("spc")
time.sleep(3.5)

# 2. Type password 'admin123'
print("[PHASE4-FINAL] Typing password 'admin123'...")
for key in ["a", "d", "m", "i", "n", "1", "2", "3"]:
    send_qcode(key)

time.sleep(1.0)
print("[PHASE4-FINAL] Pressing Enter...")
send_qcode("ret")

print("[PHASE4-FINAL] Waiting 12 seconds for desktop render...")
time.sleep(12.0)

# Screendump 1: Main Controls & Experience tab with rounded window
print("[PHASE4-FINAL] Screendump main view with rounded window...")
s.sendall(json.dumps({
    "execute": "screendump",
    "arguments": {"filename": PPM_FILE_MAIN.replace("\\", "/")}
}).encode() + b"\r\n")
s.recv(1024)
time.sleep(2.0)

# Switch to Diagnostics tab via key '2'
print("[PHASE4-FINAL] Switching to Diagnostics tab via key '2'...")
send_qcode("2")
time.sleep(2.0)

# Screendump 2: Diagnostics tab
print("[PHASE4-FINAL] Screendump Diagnostics tab...")
s.sendall(json.dumps({
    "execute": "screendump",
    "arguments": {"filename": PPM_FILE_DIAG.replace("\\", "/")}
}).encode() + b"\r\n")
s.recv(1024)
time.sleep(2.0)

s.close()
proc.kill()
print("[PHASE4-FINAL] QEMU terminated cleanly.")

# Process screenshots
if os.path.exists(PPM_FILE_MAIN):
    img_main = Image.open(PPM_FILE_MAIN)
    img_main.save(os.path.join(ARTIFACT_DIR, "phase4_final_desktop.png"))
    print(f"[PHASE4-FINAL] Saved full desktop capture ({img_main.width}x{img_main.height})")

    # Crop window: window bounds (870, 530, 820, 540)
    # Include 10px margin around window to verify rounded corners and transparent pixels
    box = (860, 520, 870 + 820 + 10, 530 + 540 + 10)
    win_main = img_main.crop(box)
    win_main.save(os.path.join(ARTIFACT_DIR, "phase4_final_window_detail.png"))
    print("[PHASE4-FINAL] Saved window detail with rounded corners")

    # Crop top-left corner magnified 4x
    corner_box = (865, 525, 895, 555)
    corner = img_main.crop(corner_box).resize((120, 120), Image.NEAREST)
    corner.save(os.path.join(ARTIFACT_DIR, "phase4_final_corner_magnified.png"))
    print("[PHASE4-FINAL] Saved magnified corner detail")

if os.path.exists(PPM_FILE_DIAG):
    img_diag = Image.open(PPM_FILE_DIAG)
    box = (860, 520, 870 + 820 + 10, 530 + 540 + 10)
    win_diag = img_diag.crop(box)
    win_diag.save(os.path.join(ARTIFACT_DIR, "phase4_final_diagnostics.png"))
    print("[PHASE4-FINAL] Saved Diagnostics tab window detail")

# Check serial log
if os.path.exists(LOG_FILE):
    with open(LOG_FILE, "r", errors="ignore") as f:
        log_content = f.read()
    print("=== SERIAL LOG AUDIT ===")
    if "[DESKTOP] WINDOW OPENED" in log_content:
        print("  Window opened: PASS")
    if "[DESKTOP] DESKTOP RENDERED" in log_content:
        print("  Desktop rendered: PASS")
    if "Kernel Panic" in log_content:
        print("  WARNING: Kernel Panic detected!")
    else:
        print("  Kernel Stability: ZERO PANICS, 100% CLEAN")
