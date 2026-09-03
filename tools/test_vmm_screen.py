import os
import sys
import time
import socket
import subprocess
from PIL import Image

QEMU_EXE = r"D:\OS-QEMU-EMU\qemu\qemu-system-x86_64.exe"
UEFI_BIOS = r"D:\OS-QEMU-EMU\qemu\share\edk2-x86_64-code.fd"
IMAGE_PATH = r"d:\Signatures_OS\build\atoms_uefi_test.img"
PPM_PATH = r"d:\Signatures_OS\build\vmm_screen.ppm"
PNG_PATH = r"d:\Signatures_OS\build\vmm_screen.png"
SERIAL_LOG = r"d:\Signatures_OS\build\vmm_screen_serial.log"

if os.path.exists(PPM_PATH):
    try: os.remove(PPM_PATH)
    except: pass
if os.path.exists(PNG_PATH):
    try: os.remove(PNG_PATH)
    except: pass
if os.path.exists(SERIAL_LOG):
    try: os.remove(SERIAL_LOG)
    except: pass

cmd = [
    QEMU_EXE,
    "-drive", f"if=pflash,format=raw,readonly=on,file={UEFI_BIOS}",
    "-drive", f"file={IMAGE_PATH},format=raw",
    "-serial", f"file:{SERIAL_LOG}",
    "-smp", "4",
    "-m", "1024M",
    "-display", "none",
    "-monitor", "telnet:127.0.0.1:4445,server,nowait",
    "-no-reboot"
]

print("[TEST] Launching QEMU UEFI with SMP (4 Cores)...")
proc = subprocess.Popen(cmd)

print("[TEST] Waiting 30 seconds for boot and PMM stress test execution...")
time.sleep(30)

try:
    print("[TEST] Connecting to QEMU monitor on port 4445...")
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect(("127.0.0.1", 4445))
    time.sleep(1.0)

    print("[TEST] Capturing screendump to PPM...")
    s.sendall(b"screendump d:/Signatures_OS/build/vmm_screen.ppm\n")
    time.sleep(1.5)
    s.sendall(b"quit\n")
    s.close()
except Exception as e:
    print(f"[TEST] Monitor communication error: {e}")

time.sleep(1)
if proc.poll() is None:
    proc.terminate()

if os.path.exists(PPM_PATH):
    print(f"[TEST] Converting {PPM_PATH} to {PNG_PATH}...")
    img = Image.open(PPM_PATH)
    img.save(PNG_PATH, "PNG")
    print(f"[TEST] SUCCESS: Saved {PNG_PATH} (Dimensions: {img.width}x{img.height})")
else:
    print("[TEST] PPM file was not created!")
