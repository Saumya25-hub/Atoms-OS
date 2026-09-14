import time
import subprocess
import socket
import json
import os
import sys
from PIL import Image

LOG_FILE = r"build\qemu_usb_real_e2e.log"
PPM_FILE = r"build\qemu_usb_real_e2e.ppm"
PNG_FILE = r"build\qemu_usb_real_e2e.png"

for p in [LOG_FILE, PPM_FILE, PNG_FILE]:
    if os.path.exists(p):
        try: os.remove(p)
        except: pass

usb_img = r"\\.\E:" if os.path.exists(r"\\.\E:") else r"build\media.img"

cmd = [
    r"D:\OS-QEMU-EMU\qemu\qemu-system-x86_64.exe",
    "-drive", r"if=pflash,format=raw,readonly=on,file=D:\OS-QEMU-EMU\qemu\share\edk2-x86_64-code.fd",
    "-drive", r"file=build\atoms_uefi_test.img,format=raw",
    "-device", "qemu-xhci,id=xhci",
    "-device", "usb-kbd,bus=xhci.0",
    "-device", "usb-mouse,bus=xhci.0",
    "-drive", f"if=none,id=usbdrive,file={usb_img},format=raw",
    "-device", "usb-storage,bus=xhci.0,drive=usbdrive",
    "-netdev", "user,id=net0",
    "-device", "e1000,netdev=net0",
    "-m", "2048M",
    "-qmp", "tcp:127.0.0.1:4480,server,nowait",
    "-serial", f"file:{LOG_FILE}",
    "-display", "none",
    "-no-reboot"
]

print("[REAL USB E2E TEST] Starting QEMU with real USB E: drive backing...")
proc = subprocess.Popen(cmd)

def read_log():
    if not os.path.exists(LOG_FILE):
        return ""
    try:
        with open(LOG_FILE, "r", errors="ignore") as f:
            return f.read()
    except Exception:
        return ""

try:
    print("[REAL USB E2E TEST] Waiting for Login Render frame in serial log...")
    ready = False
    for i in range(50):
        time.sleep(1)
        content = read_log()
        if "PROBE 3 page_login_on_render" in content:
            print(f"[REAL USB E2E TEST] Reached interactive login render at {i+1}s!")
            ready = True
            break
        if proc.poll() is not None:
            print(f"[REAL USB E2E TEST] QEMU exited early with code {proc.poll()}")
            sys.exit(1)

    if not ready:
        print("[REAL USB E2E TEST] ERROR: Login render probe not seen in 50 seconds!")
        sys.exit(1)

    time.sleep(3.0)

    print("[REAL USB E2E TEST] Connecting to QMP...")
    s = socket.socket()
    s.connect(('127.0.0.1', 4480))
    
    banner = s.recv(2048).decode(errors="ignore")
    print(f"[REAL USB E2E TEST] QMP Connected, banner length: {len(banner)}")

    s.sendall(b'{"execute": "qmp_capabilities"}\r\n')
    time.sleep(0.5)
    s.recv(2048)

    def qmp_send_key(k, hold=200):
        msg = json.dumps({
            "execute": "send-key",
            "arguments": {
                "keys": [{"type": "qcode", "data": k}],
                "hold-time": hold
            }
        }) + "\r\n"
        s.sendall(msg.encode())
        time.sleep(0.3)
        try:
            s.setblocking(False)
            while True:
                data = s.recv(1024)
                if not data: break
        except Exception:
            pass
        s.setblocking(True)

    def wait_for_log_text(pattern, timeout=10.0):
        start = time.time()
        while time.time() - start < timeout:
            if pattern in read_log():
                return True
            time.sleep(0.2)
        return False

    # 1. Send Space to unlock
    print("[REAL USB E2E TEST] Sending Space key to unlock screen...")
    qmp_send_key("spc", hold=200)
    time.sleep(4.0)

    # 2. Send Return to login
    print("[REAL USB E2E TEST] Submitting login with Return...")
    for attempt in range(5):
        qmp_send_key("ret", hold=250)
        if wait_for_log_text("[LOGIN_FLOW] AUTH_SUCCESS", 4.0):
            print(f"[REAL USB E2E TEST] AUTH_SUCCESS confirmed at attempt {attempt+1}!")
            break
        time.sleep(1.0)

    print("[REAL USB E2E TEST] Waiting for Desktop and USB storage initialization...")
    wait_for_log_text("[DESKTOP] DESKTOP RENDERED", 25.0)
    print("[REAL USB E2E TEST] Desktop rendered cleanly! Waiting 5 seconds...")
    time.sleep(5.0)

    # Screendump
    print("[REAL USB E2E TEST] Taking screendump...")
    s.sendall(json.dumps({
        "execute": "screendump",
        "arguments": {"filename": PPM_FILE.replace("\\", "/")}
    }).encode() + b"\r\n")
    time.sleep(2.0)
    s.close()

finally:
    if proc.poll() is None:
        proc.terminate()
        try:
            proc.wait(timeout=4)
        except Exception:
            proc.kill()

print("[REAL USB E2E TEST] QEMU terminated cleanly.")

if os.path.exists(PPM_FILE):
    try:
        img = Image.open(PPM_FILE)
        img.save(PNG_FILE, "PNG")
        print(f"[REAL USB E2E TEST] Screendump saved to {PNG_FILE} ({img.width}x{img.height})")
    except Exception as e:
        print(f"[REAL USB E2E TEST] Screendump conversion error: {e}")

content = read_log()
print("=================== SERIAL LOG HIGHLIGHTS ===================")
for line in content.splitlines():
    if any(w in line.lower() for w in ["msc", "bot", "scsi", "usb0", "block", "dsk", "gpt", "mbr", "fat32", "mount", "vfs", "desktop", "sync", "read capacity"]):
        print(line)
print("=============================================================")
