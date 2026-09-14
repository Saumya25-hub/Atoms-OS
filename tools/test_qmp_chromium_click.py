import time
import subprocess
import socket
import json
import os
from PIL import Image

LOG_FILE = r"build\qmp_chromium_click.log"
PPM_FILE = r"build\qmp_chromium_screen.ppm"
PNG_FILE = r"build\screen_chromium_launched.png"

for p in [LOG_FILE, PPM_FILE, PNG_FILE]:
    if os.path.exists(p):
        try: os.remove(p)
        except: pass

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
    "-qmp", "tcp:127.0.0.1:4450,server,nowait",
    "-serial", f"file:{LOG_FILE}",
    "-display", "none"
])

print("[CHROMIUM TEST] Waiting for Login Supervisor Loop in serial log...")
for i in range(35):
    time.sleep(1)
    if os.path.exists(LOG_FILE):
        with open(LOG_FILE, "r", errors="ignore") as f:
            content = f.read()
            if "Entering Interactive Login Supervisor Loop" in content:
                print(f"[CHROMIUM TEST] Reached login loop in {i+1}s!")
                break

time.sleep(1.0)

s = socket.socket()
s.connect(('127.0.0.1', 4450))
s.recv(1024)
s.sendall(b'{"execute": "qmp_capabilities"}\r\n')
s.recv(1024)

def send_qcode(k, hold=180):
    msg = json.dumps({
        "execute": "send-key",
        "arguments": {
            "keys": [{"type": "qcode", "data": k}],
            "hold-time": hold
        }
    }) + "\r\n"
    s.sendall(msg.encode())
    s.recv(1024)
    time.sleep(0.25)

def mouse_click_at(x_pix, y_pix):
    x_norm = int(x_pix / 2560.0 * 32767)
    y_norm = int(y_pix / 1600.0 * 32767)
    move_msg = json.dumps({
        "execute": "input-send-event",
        "arguments": {
            "events": [
                {"type": "abs", "data": {"axis": "x", "value": x_norm}},
                {"type": "abs", "data": {"axis": "y", "value": y_norm}}
            ]
        }
    }) + "\r\n"
    s.sendall(move_msg.encode())
    s.recv(1024)
    time.sleep(0.1)
    click_down = json.dumps({
        "execute": "input-send-event",
        "arguments": {"events": [{"type": "btn", "data": {"button": "left", "down": True}}]}
    }) + "\r\n"
    s.sendall(click_down.encode())
    s.recv(1024)
    time.sleep(0.15)
    click_up = json.dumps({
        "execute": "input-send-event",
        "arguments": {"events": [{"type": "btn", "data": {"button": "left", "down": False}}]}
    }) + "\r\n"
    s.sendall(click_up.encode())
    s.recv(1024)
    time.sleep(0.2)

def enter_credentials():
    print("[CHROMIUM TEST] Typing password 'admin123'...")
    for key in ["a", "d", "m", "i", "n", "1", "2", "3"]:
        send_qcode(key, hold=180)
    time.sleep(0.5)
    print("[CHROMIUM TEST] Submitting password (ret + click sign-in button)...")
    send_qcode("ret", hold=200)
    mouse_click_at(1280, 905)

# 1. Dismiss lock screen: click center and send space
print("[CHROMIUM TEST] Dismissing lock screen...")
mouse_click_at(1280, 800)
send_qcode("spc", hold=200)
time.sleep(3.0)

# 2. Type password
enter_credentials()

# Wait for DESKTOP_VISIBLE with retry
print("[CHROMIUM TEST] Waiting for DESKTOP_VISIBLE in serial log...")
desktop_ok = False
for attempt in range(25):
    time.sleep(1.0)
    if os.path.exists(LOG_FILE):
        with open(LOG_FILE, "r", errors="ignore") as f:
            c = f.read()
            if "DESKTOP_VISIBLE" in c or "Spawning First Ring 3 User Process" in c or "[DESKTOP]" in c:
                print(f"[CHROMIUM TEST] Desktop active after {attempt+1}s!")
                desktop_ok = True
                break
            if "Incorrect Password" in c and attempt >= 4:
                print("[CHROMIUM TEST] Detected incorrect password! Clearing and retrying...")
                for _ in range(12):
                    send_qcode("backspace", hold=100)
                time.sleep(0.5)
                enter_credentials()

if not desktop_ok:
    print("[CHROMIUM TEST] Desktop transition not detected, proceeding anyway...")

time.sleep(2.5)

# Calculate normalized coordinates for Chromium dock icon (slot 7)
# Screen is 2560x1600. Center of slot 7 is at (1337, 1562)
print("[CHROMIUM TEST] Clicking Chromium dock icon at (1337, 1562)...")
mouse_click_at(1337, 1562)

print("[CHROMIUM TEST] Waiting 8 seconds for Chromium process to spawn and initialize...")
time.sleep(8.0)

# Screendump
print("[CHROMIUM TEST] Dumping screen...")
screendump_cmd = json.dumps({
    "execute": "screendump",
    "arguments": {"filename": PPM_FILE.replace("\\", "/")}
}) + "\r\n"
s.sendall(screendump_cmd.encode())
time.sleep(1.5)

s.close()
proc.kill()

if os.path.exists(PPM_FILE):
    img = Image.open(PPM_FILE)
    img.save(PNG_FILE, "PNG")
    print(f"[CHROMIUM TEST] SUCCESS: Saved screenshot to {PNG_FILE} ({img.width}x{img.height})")

if os.path.exists(LOG_FILE):
    with open(LOG_FILE, "r", errors="ignore") as f:
        content = f.read()
        print("=== CHROMIUM LAUNCH TELEMETRY AUDIT ===")
        found_chromium = False
        lines = content.splitlines()
        launch_idx = -1
        for idx, line in enumerate(lines):
            if "[CHROMIUM]" in line or "chromium_browser" in line:
                launch_idx = idx
                break
        if launch_idx != -1:
            print(f"[CHROMIUM AUDIT] Found Chromium launch at log line {launch_idx+1}:")
            for l in lines[max(0, launch_idx - 2):]:
                print(l)
        else:
            print("[CHROMIUM AUDIT] No [CHROMIUM] marker found. Printing last 40 lines:")
            for line in lines[-40:]:
                print(line)
