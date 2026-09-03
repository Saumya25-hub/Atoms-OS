import os
import sys
import time
import socket
import subprocess
from PIL import Image

QEMU_EXE = r"D:\OS-QEMU-EMU\qemu\qemu-system-x86_64.exe"
UEFI_BIOS = r"D:\OS-QEMU-EMU\qemu\share\edk2-x86_64-code.fd"
IMAGE_PATH = r"d:\Signatures_OS\build\atoms_uefi_test.img"
PPM_PATH = r"d:\Signatures_OS\build\vfs_screen.ppm"
PNG_PATH = r"C:\Users\Saumya Chaudhari\.gemini\antigravity-ide\brain\66a3ebb8-ccdd-4fa9-b4d4-b0b64eceb23c\vfs_lifecycle_dashboard.png"
SERIAL_LOG = r"d:\Signatures_OS\build\uefi_forensic_serial.log"

if os.path.exists(PPM_PATH):
    try: os.remove(PPM_PATH)
    except: pass
if os.path.exists(PNG_PATH):
    try: os.remove(PNG_PATH)
    except: pass

cmd = [
    QEMU_EXE,
    "-drive", f"if=pflash,format=raw,readonly=on,file={UEFI_BIOS}",
    "-drive", f"file={IMAGE_PATH},format=raw",
    "-serial", f"file:{SERIAL_LOG}",
    "-m", "1024M",
    "-display", "none",
    "-monitor", "telnet:127.0.0.1:4445,server,nowait",
    "-no-reboot"
]

print("[CAPTURE] Launching QEMU UEFI...")
proc = subprocess.Popen(cmd)

print("[CAPTURE] Waiting 45 seconds for all 2050 cycles and final telemetry to complete...")
time.sleep(45)

try:
    print("[CAPTURE] Connecting to QEMU monitor on port 4445...")
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect(("127.0.0.1", 4445))
    time.sleep(1.0)

    print("[CAPTURE] Capturing screendump to PPM...")
    s.sendall(b"screendump d:/Signatures_OS/build/vfs_screen.ppm\n")
    time.sleep(1.5)
    s.sendall(b"quit\n")
    s.close()
except Exception as e:
    print(f"[CAPTURE] Monitor communication error: {e}")

time.sleep(1)
if proc.poll() is None:
    proc.terminate()

if os.path.exists(PPM_PATH):
    print(f"[CAPTURE] Converting {PPM_PATH} to {PNG_PATH}...")
    img = Image.open(PPM_PATH)
    img.save(PNG_PATH, "PNG")
    print(f"[CAPTURE] SUCCESS: Saved {PNG_PATH} (Dimensions: {img.width}x{img.height})")
else:
    print("[CAPTURE] PPM file was not created!")
