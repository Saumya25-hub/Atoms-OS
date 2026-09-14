import time
import socket
import json
import subprocess
import os
from PIL import Image

LOG_FILE = r"build\qmp_diag_test.log"
PPM_DIAG = r"build\screen_diag.ppm"
PNG_DIAG = r"artifacts\desktop_diagnostics_detail.png"

for p in [LOG_FILE, PPM_DIAG]:
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
    "-qmp", "tcp:127.0.0.1:4455,server,nowait",
    "-serial", f"file:{LOG_FILE}",
    "-display", "none"
])

print("[QMP] Waiting for Login Supervisor Loop in serial log...")
for i in range(35):
    time.sleep(1)
    if os.path.exists(LOG_FILE):
        with open(LOG_FILE, "r", errors="ignore") as f:
            if "Entering Interactive Login Supervisor Loop" in f.read():
                print(f"[QMP] Reached login loop in {i+1}s!")
                break

time.sleep(1.0)
s = socket.socket()
s.connect(('127.0.0.1', 4455))
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

def click_abs(x, y):
    nx = int(x * 32767 / 1024)
    ny = int(y * 32767 / 768)
    msg_move = json.dumps({
        "execute": "input-send-event",
        "arguments": {"events": [
            {"type": "abs", "data": {"axis": "x", "value": nx}},
            {"type": "abs", "data": {"axis": "y", "value": ny}}
        ]}
    }) + "\r\n"
    s.sendall(msg_move.encode())
    s.recv(1024)
    time.sleep(0.1)

    msg_down = json.dumps({
        "execute": "input-send-event",
        "arguments": {"events": [
            {"type": "btn", "data": {"button": "left", "down": True}}
        ]}
    }) + "\r\n"
    s.sendall(msg_down.encode())
    s.recv(1024)
    time.sleep(0.1)

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
print("[QMP] Dismissing lock screen...")
send_qcode("spc")
time.sleep(3.5)

# 2. Type password
print("[QMP] Typing password...")
for key in ["a", "d", "m", "i", "n", "1", "2", "3"]:
    send_qcode(key)

time.sleep(1.0)
print("[QMP] Pressing Enter...")
send_qcode("ret")

print("[QMP] Waiting 10 seconds for desktop...")
time.sleep(10.0)

# Window position on 1024x768 screen: wx = (1024 - 820)/2 = 102, wy = (768 - 540)/2 = 114
# Tab 1 (Diagnostics): x in [wx + 10, wx + 180] = [112, 282], y in [wy + 72 + 44, wy + 72 + 80] = [230, 266]
print("[QMP] Clicking Diagnostics tab at (180, 245)...")
click_abs(180, 245)
time.sleep(2.0)

# Screendump
print("[QMP] Screendump...")
screendump_cmd = json.dumps({
    "execute": "screendump",
    "arguments": {"filename": PPM_DIAG.replace("\\", "/")}
}) + "\r\n"
s.sendall(screendump_cmd.encode())
time.sleep(1.5)

s.close()
proc.kill()

if os.path.exists(PPM_DIAG):
    im = Image.open(PPM_DIAG)
    box = (int(1280 - 460), int(800 - 320), int(1280 + 460), int(800 + 320))
    cr = im.crop(box)
    cr.save(PNG_DIAG)
    cr.save(r"C:\Users\Saumya Chaudhari\.gemini\antigravity-ide\brain\09feded4-1087-4ee2-a7c5-fcd836a51008\desktop_diagnostics_detail.png")
    print(f"[QMP] SUCCESS: Saved diagnostics tab capture to {PNG_DIAG}")
