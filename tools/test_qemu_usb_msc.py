import os
import subprocess
import time
import sys

QEMU_EXE = r"D:\OS-QEMU-EMU\qemu\qemu-system-x86_64.exe"
UEFI_BIOS = r"D:\OS-QEMU-EMU\qemu\share\edk2-x86_64-code.fd"
LOG_FILE = r"build\qemu_usb_msc_test.log"

if os.path.exists(LOG_FILE):
    try:
        os.remove(LOG_FILE)
    except Exception:
        pass

args = [
    QEMU_EXE,
    "-drive", f"if=pflash,format=raw,readonly=on,file={UEFI_BIOS}",
    "-drive", "file=build\\atoms_uefi_test.img,format=raw",
    "-device", "qemu-xhci,id=xhci",
    "-device", "usb-kbd,bus=xhci.0",
    "-device", "usb-mouse,bus=xhci.0",
    "-drive", "if=none,id=usbdrive,file=build\\media.img,format=raw",
    "-device", "usb-storage,bus=xhci.0,drive=usbdrive",
    "-serial", f"file:{LOG_FILE}",
    "-m", "2048M",
    "-display", "none",
    "-no-reboot"
]

print("[TEST RUNNER] Launching QEMU with xHCI USB Mass Storage Flash Drive...")
proc = subprocess.Popen(args)

try:
    for i in range(60):
        time.sleep(1)
        if proc.poll() is not None:
            break
        if os.path.exists(LOG_FILE):
            with open(LOG_FILE, "r", encoding="utf-8", errors="ignore") as f:
                txt = f.read()
                if "[DESKTOP_SHELL]" in txt:
                    print(f"[TEST RUNNER] Desktop Shell reached at {i}s!")
                    time.sleep(3)
                    break
finally:
    if proc.poll() is None:
        proc.terminate()
        try:
            proc.wait(timeout=3)
        except Exception:
            proc.kill()

print(f"[TEST RUNNER] QEMU finished. Reading log: {LOG_FILE}")
if os.path.exists(LOG_FILE):
    with open(LOG_FILE, "r", encoding="utf-8", errors="ignore") as f:
        content = f.read()
    print("=================== SERIAL LOG OUTPUT ===================")
    print(content)
    print("=========================================================")
else:
    print("[ERROR] Log file not found!")
