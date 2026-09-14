import time
import subprocess
import socket
import json
import os
import sys
from PIL import Image

LOG_FILE = r"build\qemu_usb_msc_e2e.log"
PPM_FILE = r"build\qemu_usb_msc_e2e.ppm"
PNG_FILE = r"build\qemu_usb_msc_e2e.png"

for p in [LOG_FILE, PPM_FILE, PNG_FILE]:
    if os.path.exists(p):
        try: os.remove(p)
        except: pass

cmd = [
    r"D:\OS-QEMU-EMU\qemu\qemu-system-x86_64.exe",
    "-drive", r"if=pflash,format=raw,readonly=on,file=D:\OS-QEMU-EMU\qemu\share\edk2-x86_64-code.fd",
    "-drive", r"file=build\atoms_uefi_test.img,format=raw",
    "-device", "qemu-xhci,id=xhci",
    "-device", "usb-kbd,bus=xhci.0",
    "-device", "usb-mouse,bus=xhci.0",
    "-drive", "if=none,id=usbdrive,file=build\\media.img,format=raw",
    "-device", "usb-storage,bus=xhci.0,drive=usbdrive",
    "-netdev", "user,id=net0",
    "-device", "e1000,netdev=net0",
    "-m", "2048M",
    "-qmp", "tcp:127.0.0.1:4475,server,nowait",
    "-serial", f"file:{LOG_FILE}",
    "-display", "none",
    "-no-reboot"
]

print("[MSC E2E TEST] Starting QEMU...")
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
    print("[MSC E2E TEST] Waiting for Login Render frame in serial log...")
    ready = False
    for i in range(50):
        time.sleep(1)
        content = read_log()
        if "PROBE 3 page_login_on_render" in content:
            print(f"[MSC E2E TEST] Reached interactive login render at {i+1}s!")
            ready = True
            break
        if proc.poll() is not None:
            print(f"[MSC E2E TEST] QEMU exited early with code {proc.poll()}")
            sys.exit(1)

    if not ready:
        print("[MSC E2E TEST] ERROR: Login render probe not seen in 50 seconds!")
        sys.exit(1)

    time.sleep(3.0)  # Allow first frame to settle completely

    print("[MSC E2E TEST] Connecting to QMP...")
    s = socket.socket()
    s.connect(('127.0.0.1', 4475))
    
    # Read initial greeting banner
    banner = s.recv(2048).decode(errors="ignore")
    print(f"[MSC E2E TEST] QMP Connected, banner length: {len(banner)}")

    # Send qmp_capabilities
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
        # Flush any responses/events
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

    # 1. Send Space to dismiss lock screen
    print("[MSC E2E TEST] Sending Space key to unlock screen...")
    qmp_send_key("spc")
    if wait_for_log_text("0x2C", 8.0):
        print("[MSC E2E TEST] Confirmed: Space key recognized by USB HID driver!")
    else:
        print("[MSC E2E TEST] Warning: 0x2C not observed yet, retrying space...")
        qmp_send_key("spc")

    # Wait for 400ms transition animation to complete
    print("[MSC E2E TEST] Waiting 2s for lock screen transition to finish...")
    time.sleep(2.5)

    # 2. Type password credentials
    print("[MSC E2E TEST] Typing 'admin123' sequentially...")
    key_map = [
        ("a", "0x4"),
        ("d", "0x7"),
        ("m", "0x10"),
        ("i", "0xC"),
        ("n", "0x11"),
        ("1", "0x1E"),
        ("2", "0x1F"),
        ("3", "0x20")
    ]

    for char, hex_usage in key_map:
        qmp_send_key(char)
        time.sleep(0.5)

    print("[MSC E2E TEST] Submitting credentials with Return...")
    qmp_send_key("ret")

    print("[MSC E2E TEST] Monitoring for Desktop handoff and Storage initialization...")
    desktop_reached = False
    for i in range(25):
        time.sleep(1)
        content = read_log()
        if "[BLOCK] usb0 registered" in content or "Initializing Disk Manager" in content or "[DSK]" in content:
            print(f"[MSC E2E TEST] SUCCESS: Disk Manager / USB0 storage activity detected at +{i+1}s!")
            desktop_reached = True
            break
        if "DESKTOP_LAUNCH_BEGIN" in content:
            print(f"[MSC E2E TEST] Desktop launch initiated at +{i+1}s...")

    # Wait an additional 5s for full partition scanning & mounting
    time.sleep(5.0)

    # Screendump
    print("[MSC E2E TEST] Taking screendump...")
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

print("[MSC E2E TEST] QEMU terminated cleanly.")

if os.path.exists(PPM_FILE):
    try:
        img = Image.open(PPM_FILE)
        img.save(PNG_FILE, "PNG")
        print(f"[MSC E2E TEST] Screendump saved to {PNG_FILE} ({img.width}x{img.height})")
    except Exception as e:
        print(f"[MSC E2E TEST] Screendump conversion error: {e}")

content = read_log()
print("=================== SERIAL LOG HIGHLIGHTS ===================")
for line in content.splitlines():
    if any(w in line.lower() for w in ["msc", "bot", "scsi", "usb0", "block", "dsk", "gpt", "mbr", "fat32", "mount", "vfs", "disk"]):
        print(line)
print("=============================================================")
