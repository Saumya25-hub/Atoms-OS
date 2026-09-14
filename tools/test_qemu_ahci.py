import os
import sys
import time
import socket
import subprocess
from PIL import Image

QEMU_EXE = r"D:\OS-QEMU-EMU\qemu\qemu-system-x86_64.exe"
UEFI_BIOS = r"D:\OS-QEMU-EMU\qemu\share\edk2-x86_64-code.fd"
IMAGE_PATH = r"d:\Signatures_OS\build\atoms_uefi_test.img"
SERIAL_LOG = r"d:\Signatures_OS\build\ahci_qemu_serial.log"
PPM_PATH = r"d:\Signatures_OS\build\ahci_qemu_screen.ppm"
PNG_PATH = r"C:\Users\Saumya Chaudhari\.gemini\antigravity-ide\brain\66a3ebb8-ccdd-4fa9-b4d4-b0b64eceb23c\ahci_qemu_dashboard.png"

if os.path.exists(SERIAL_LOG):
    try: os.remove(SERIAL_LOG)
    except: pass

cmd = [
    QEMU_EXE,
    "-drive", f"if=pflash,format=raw,readonly=on,file={UEFI_BIOS}",
    "-device", "ahci,id=ahci0",
    "-drive", f"id=sata0,file={IMAGE_PATH},format=raw,if=none",
    "-device", "ide-hd,drive=sata0,bus=ahci0.0",
    "-serial", f"file:{SERIAL_LOG}",
    "-m", "1024M",
    "-device", "qemu-xhci",
    "-device", "usb-kbd",
    "-device", "usb-mouse",
    "-netdev", "user,id=net0",
    "-device", "e1000,netdev=net0",
    "-display", "none",
    "-monitor", "telnet:127.0.0.1:4448,server,nowait",
    "-no-reboot"
]

print("[TEST] Launching QEMU UEFI with Native AHCI SATA Controller & Drive...")
proc = subprocess.Popen(cmd)

print("[TEST] Waiting 16 seconds for boot and AHCI bring-up...")
time.sleep(16)

try:
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect(("127.0.0.1", 4448))
    time.sleep(0.5)

    print("[TEST] Capturing screen dump...")
    s.sendall(f"screendump {PPM_PATH}\n".encode("ascii"))
    time.sleep(1.5)
    s.sendall(b"quit\n")
    s.close()
except Exception as e:
    print(f"[TEST] Monitor connection exception: {e}")

if proc.poll() is None:
    proc.terminate()

if os.path.exists(PPM_PATH):
    img = Image.open(PPM_PATH)
    img.save(PNG_PATH, "PNG")
    print(f"[TEST] Screen dump saved successfully to: {PNG_PATH}")

if os.path.exists(SERIAL_LOG):
    print("\n================== AHCI SERIAL COM1 LOG OUTPUT ==================")
    with open(SERIAL_LOG, "r", errors="ignore") as f:
        print(f.read())
    print("=================================================================\n")
