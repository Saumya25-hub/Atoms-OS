#!/usr/bin/env python3
import os
import sys
import time
import socket
import json
import subprocess
from PIL import Image

QEMU_EXE = r"D:\OS-QEMU-EMU\qemu\qemu-system-x86_64.exe"
OVMF_BIOS = r"D:\OS-QEMU-EMU\qemu\share\edk2-x86_64-code.fd"
GPT_IMG = r"build\atoms_uefi_test.img"
SERIAL_LOG = r"build\capture_playback_serial.log"
PPM_FILE = r"build\media_playback_screen.ppm"
ARTIFACT_DIR = r"C:\Users\Saumya Chaudhari\.gemini\antigravity-ide\brain\09feded4-1087-4ee2-a7c5-fcd836a51008"
PNG_MAIN = os.path.join(ARTIFACT_DIR, "media_player_playback.png")
PNG_DETAIL = os.path.join(ARTIFACT_DIR, "media_player_playback_detail.png")
QMP_PORT = 4472

for p in [SERIAL_LOG, PPM_FILE]:
    if os.path.exists(p):
        try: os.remove(p)
        except: pass

cmd = [
    QEMU_EXE,
    "-drive", f"if=pflash,format=raw,readonly=on,file={OVMF_BIOS}",
    "-drive", f"file={GPT_IMG},format=raw",
    "-device", "qemu-xhci",
    "-device", "usb-mouse",
    "-device", "usb-kbd",
    "-device", "intel-hda",
    "-device", "hda-duplex",
    "-netdev", "user,id=net0",
    "-device", "e1000,netdev=net0",
    "-qmp", f"tcp:127.0.0.1:{QMP_PORT},server,nowait",
    "-serial", f"file:{SERIAL_LOG}",
    "-m", "2048M",
    "-display", "none",
    "-no-reboot"
]

print("[CAPTURE] Launching QEMU...")
proc = subprocess.Popen(cmd)

for i in range(45):
    time.sleep(1)
    if os.path.exists(SERIAL_LOG):
        with open(SERIAL_LOG, "r", errors="ignore") as f:
            if "Entering Interactive Login Supervisor Loop" in f.read():
                print(f"[CAPTURE] Login loop reached in {i+1}s!")
                break

time.sleep(1.5)
s = socket.socket()
s.connect(("127.0.0.1", QMP_PORT))
s.recv(1024)
s.sendall(b'{"execute": "qmp_capabilities"}\r\n')
s.recv(1024)

def send_key(k, hold=150):
    msg = json.dumps({"execute": "send-key", "arguments": {"keys": [{"type": "qcode", "data": k}], "hold-time": hold}}) + "\r\n"
    s.sendall(msg.encode())
    s.recv(1024)
    time.sleep(0.3)

send_key("spc")
time.sleep(3.5)
for ch in ["a", "d", "m", "i", "n", "1", "2", "3"]:
    send_key(ch)
time.sleep(0.5)
send_key("ret")

print("[CAPTURE] Logged in! Waiting for video playback frames...")
for i in range(40):
    time.sleep(1.0)
    if os.path.exists(SERIAL_LOG):
        with open(SERIAL_LOG, "r", errors="ignore") as f:
            c = f.read()
        if "frame 0 decoded" in c and "Bitmap Blit Result: SUCCESS" in c:
            print(f"[CAPTURE] Video playing actively after {i+1}s!")
            break

time.sleep(2.0)
dump_cmd = json.dumps({"execute": "screendump", "arguments": {"filename": PPM_FILE.replace("\\", "/")}}) + "\r\n"
s.sendall(dump_cmd.encode())
s.recv(1024)
time.sleep(2.0)

s.close()
proc.kill()
try: proc.wait(timeout=3)
except: pass

if os.path.exists(PPM_FILE):
    img = Image.open(PPM_FILE)
    img.save(PNG_MAIN)
    box = (90, 40, 100 + 1280 + 20, 50 + 720 + 20)
    crop = img.crop(box)
    crop.save(PNG_DETAIL)
    print(f"[CAPTURE] SUCCESS! Saved desktop capture -> {PNG_MAIN}")
    print(f"[CAPTURE] SUCCESS! Saved video window detail -> {PNG_DETAIL}")
else:
    print("[CAPTURE] ERROR: PPM file not generated!")
