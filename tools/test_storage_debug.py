import os
import sys
import time
import socket
import subprocess
from PIL import Image

QEMU_EXE = r"D:\OS-QEMU-EMU\qemu\qemu-system-x86_64.exe"
UEFI_BIOS = r"D:\OS-QEMU-EMU\qemu\share\edk2-x86_64-code.fd"
IMAGE_PATH = r"d:\Signatures_OS\build\atoms_uefi_test.img"
SERIAL_LOG = r"d:\Signatures_OS\build\storage_bringup_serial.log"

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

print("[TEST] Launching QEMU UEFI with Storage Bring-Up Debug Suite...")
proc = subprocess.Popen(cmd)

print("[TEST] Waiting 15 seconds for boot and storage diagnostic dashboard...")
time.sleep(15)

try:
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect(("127.0.0.1", 4447))
    time.sleep(0.5)

    ppm_path = r"d:\Signatures_OS\build\screen_storage_debug.ppm"
    print("[TEST] Capturing screen dump...")
    s.sendall(f"screendump {ppm_path}\n".encode("ascii"))
    time.sleep(1.5)
    s.sendall(b"quit\n")
    s.close()
except Exception as e:
    print(f"[TEST] Monitor connection exception: {e}")

if proc.poll() is None:
    proc.terminate()

png_path = r"C:\Users\Saumya Chaudhari\.gemini\antigravity-ide\brain\66a3ebb8-ccdd-4fa9-b4d4-b0b64eceb23c\storage_forensic_dashboard.png"
if os.path.exists(ppm_path):
    img = Image.open(ppm_path)
    img.save(png_path, "PNG")
    print(f"[TEST] Screen dump saved successfully to: {png_path}")

if os.path.exists(SERIAL_LOG):
    print("\n================== SERIAL COM1 LOG OUTPUT ==================")
    with open(SERIAL_LOG, "r", errors="ignore") as f:
        lines = f.readlines()
        for line in lines[-60:]:
            sys.stdout.write(line)
    print("============================================================\n")
