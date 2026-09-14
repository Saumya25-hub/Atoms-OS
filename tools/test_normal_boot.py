import os
import sys
import time
import socket
import subprocess
from PIL import Image

QEMU_EXE = r"D:\OS-QEMU-EMU\qemu\qemu-system-x86_64.exe"
UEFI_BIOS = r"D:\OS-QEMU-EMU\qemu\share\edk2-x86_64-code.fd"
IMAGE_PATH = r"d:\Signatures_OS\build\atoms_uefi_test.img"
SERIAL_LOG = r"d:\Signatures_OS\build\normal_boot_serial.log"

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
    "-monitor", "telnet:127.0.0.1:4446,server,nowait",
    "-no-reboot"
]

print("[BOOT] Launching QEMU UEFI in normal boot mode...")
proc = subprocess.Popen(cmd)

print("[BOOT] Waiting 25 seconds for boot splash and login screen...")
time.sleep(25)

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect(("127.0.0.1", 4446))
time.sleep(0.5)

# Capture login screen first
print("[BOOT] Capturing login screen...")
s.sendall(b"screendump d:/Signatures_OS/build/screen_login.ppm\n")
time.sleep(1.0)

# Type admin123 and enter
print("[BOOT] Typing password 'admin123' and pressing Enter...")
for k in ["a", "d", "m", "i", "n", "1", "2", "3", "ret"]:
    s.sendall(f"sendkey {k}\n".encode("ascii"))
    time.sleep(0.15)

print("[BOOT] Waiting 5 seconds for desktop transition...")
time.sleep(5)

# Capture desktop screen
print("[BOOT] Capturing desktop screen...")
s.sendall(b"screendump d:/Signatures_OS/build/screen_desktop.ppm\n")
time.sleep(1.5)
s.sendall(b"quit\n")
s.close()

if proc.poll() is None:
    proc.terminate()

ppm_login = r"d:/Signatures_OS/build/screen_login.ppm"
png_login = r"d:/Signatures_OS/build/screen_login.png"
if os.path.exists(ppm_login):
    img = Image.open(ppm_login)
    img.save(png_login, "PNG")
    print(f"[BOOT] Login screen saved: {png_login}")

ppm_desktop = r"d:/Signatures_OS/build/screen_desktop.ppm"
png_desktop = r"d:/Signatures_OS/build/screen_desktop.png"
if os.path.exists(ppm_desktop):
    img = Image.open(ppm_desktop)
    img.save(png_desktop, "PNG")
    print(f"[BOOT] Desktop screen saved: {png_desktop}")
