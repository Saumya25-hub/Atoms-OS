import time
import subprocess
import socket
import json
import os
from PIL import Image

LOG_FILE = r"build\qmp_login_test.log"
PPM_FILE = r"build\qmp_screen.ppm"
PNG_FILE = r"build\screen_desktop_active.png"

for p in [LOG_FILE, PPM_FILE]:
    if os.path.exists(p):
        try: os.remove(p)
        except: pass

proc = subprocess.Popen([
    r"D:\OS-QEMU-EMU\qemu\qemu-system-x86_64.exe",
    "-drive", r"if=pflash,format=raw,readonly=on,file=D:\OS-QEMU-EMU\qemu\share\edk2-x86_64-code.fd",
    "-drive", r"file=build\atoms_uefi_test.img,format=raw",
    "-device", "qemu-xhci",
    "-device", "usb-kbd",
    "-device", "usb-mouse",
    "-netdev", "user,id=net0",
    "-device", "e1000,netdev=net0",
    "-m", "2048M",
    "-qmp", "tcp:127.0.0.1:4449,server,nowait",
    "-serial", f"file:{LOG_FILE}",
    "-display", "none"
])

print("[QMP] Waiting for Login Supervisor Loop in serial log...")
for i in range(35):
    time.sleep(1)
    if os.path.exists(LOG_FILE):
        with open(LOG_FILE, "r", errors="ignore") as f:
            content = f.read()
            if "Entering Interactive Login Supervisor Loop" in content:
                print(f"[QMP] Reached login loop in {i+1}s!")
                break

time.sleep(1.0)

s = socket.socket()
s.connect(('127.0.0.1', 4449))
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
print("[QMP] Dismissing lock screen...")
send_qcode("spc")
time.sleep(3.5)

# 2. Type password 'admin123'
print("[QMP] Typing password...")
for key in ["a", "d", "m", "i", "n", "1", "2", "3"]:
    send_qcode(key)

time.sleep(1.0)
print("[QMP] Pressing Enter...")
send_qcode("ret")

print("[QMP] Waiting 12 seconds for desktop transition...")
time.sleep(12.0)

# Screendump
print("[QMP] Dumping screen...")
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
    print(f"[QMP] SUCCESS: Saved desktop capture to {PNG_FILE} ({img.width}x{img.height})")

if os.path.exists(LOG_FILE):
    with open(LOG_FILE, "r", errors="ignore") as f:
        lines = f.readlines()
        print("=== LAST 25 LINES OF SERIAL LOG ===")
        for l in lines[-25:]:
            print(l.strip())
