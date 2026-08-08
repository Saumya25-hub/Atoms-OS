import os
import sys
import time
import socket
import subprocess
from PIL import Image

QEMU_EXE = r"D:\OS-QEMU-EMU\qemu\qemu-system-x86_64.exe"
UEFI_BIOS = r"D:\OS-QEMU-EMU\qemu\share\edk2-x86_64-code.fd"
IMAGE_PATH = r"d:\Signatures_OS\build\atoms_uefi_test.img"
PPM_PATH = r"d:\Signatures_OS\build\qemu_screen.ppm"
PNG_PATH = r"d:\Signatures_OS\build\qemu_screen.png"
ARTIFACT_PNG_PATH = r"C:\Users\Saumya Chaudhari\.gemini\antigravity-ide\brain\0d55c911-de87-463b-b203-b27ebd7560fc\qemu_screendump.png"

def capture():
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
        "-smp", "4",
        "-m", "512M",
        "-display", "none",
        "-monitor", "telnet:127.0.0.1:4444,server,nowait",
        "-no-reboot"
    ]

    print("[QEMU CAPTURE] Launching QEMU in 4-Core SMP UEFI Mode...")
    proc = subprocess.Popen(cmd)

    print("[QEMU CAPTURE] Waiting 15 seconds for OVMF 4-core SMP & ABDE V2.5 Dashboard Render...")
    time.sleep(15)

    try:
        print("[QEMU CAPTURE] Connecting to QEMU monitor telnet (127.0.0.1:4444)...")
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect(("127.0.0.1", 4444))
        time.sleep(0.5)

        print("[QEMU CAPTURE] Sending screendump command...")
        s.sendall(b"screendump d:/Signatures_OS/build/qemu_screen.ppm\n")
        time.sleep(1.0)
        s.sendall(b"quit\n")
        s.close()
    except Exception as e:
        print(f"[QEMU CAPTURE] Error communicating with QEMU monitor: {e}")

    time.sleep(1)
    if proc.poll() is None:
        proc.terminate()

    if os.path.exists(PPM_PATH):
        print(f"[QEMU CAPTURE] Converting PPM {PPM_PATH} to PNG {PNG_PATH}...")
        img = Image.open(PPM_PATH)
        img.save(PNG_PATH, "PNG")
        img.save(ARTIFACT_PNG_PATH, "PNG")
        print(f"[SUCCESS] QEMU Screenshot saved to:\n  Local: {PNG_PATH}\n  Artifact: {ARTIFACT_PNG_PATH}")
        return True
    else:
        print("[ERROR] PPM file was not created by QEMU.")
        return False

if __name__ == "__main__":
    success = capture()
    sys.exit(0 if success else 1)
