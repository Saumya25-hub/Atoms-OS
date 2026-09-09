import os
import sys
import time
import socket
import subprocess
from PIL import Image

QEMU_EXE = r"D:\OS-QEMU-EMU\qemu\qemu-system-x86_64.exe"
UEFI_BIOS = r"D:\OS-QEMU-EMU\qemu\share\edk2-x86_64-code.fd"
IMAGE_PATH = r"d:\Signatures_OS\build\atoms_uefi_test.img"
SERIAL_LOG = r"d:\Signatures_OS\build\chromium_test_serial.log"

if os.path.exists(SERIAL_LOG):
    try: os.remove(SERIAL_LOG)
    except: pass

cmd = [
    QEMU_EXE,
    "-drive", f"if=pflash,format=raw,readonly=on,file={UEFI_BIOS}",
    "-drive", f"file={IMAGE_PATH},format=raw",
    "-serial", f"file:{SERIAL_LOG}",
    "-m", "1024M",
    "-device", "qemu-xhci",
    "-device", "usb-kbd",
    "-device", "usb-mouse",
    "-netdev", "user,id=net0",
    "-device", "e1000,netdev=net0",
    "-display", "none",
    "-monitor", "telnet:127.0.0.1:4447,server,nowait",
    "-no-reboot"
]

print("[TEST] Launching QEMU UEFI...")
proc = subprocess.Popen(cmd)

print("[TEST] Waiting 25 seconds for boot splash and lock/login screen...")
time.sleep(25)

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect(("127.0.0.1", 4447))
time.sleep(0.5)

# Step 1: Send Space or Return to dismiss lock screen if locked
print("[TEST] Dismissing lock screen...")
s.sendall(b"sendkey spc\n")
time.sleep(2.0)

# Step 2: Type admin123 and press Enter
print("[TEST] Typing password 'admin123' and pressing Enter...")
for k in ["a", "d", "m", "i", "n", "1", "2", "3", "ret"]:
    s.sendall(f"sendkey {k}\n".encode("ascii"))
    time.sleep(0.3)

print("[TEST] Waiting 10 seconds for desktop to initialize and load...")
time.sleep(10)

# Step 3: Capture Desktop screendump
print("[TEST] Capturing desktop screen...")
s.sendall(b"screendump d:/Signatures_OS/build/screen_desktop_active.ppm\n")
time.sleep(2.0)
s.sendall(b"quit\n")
s.close()

if proc.poll() is None:
    proc.terminate()

ppm = r"d:/Signatures_OS/build/screen_desktop_active.ppm"
png = r"d:/Signatures_OS/build/screen_desktop_active.png"
if os.path.exists(ppm):
    img = Image.open(ppm)
    img.save(png, "PNG")
    print(f"[TEST] Desktop screenshot saved: {png}")
else:
    print("[TEST] PPM not found!")
