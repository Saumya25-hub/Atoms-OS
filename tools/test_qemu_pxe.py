import subprocess
import time
import os
import sys

QEMU_EXE = r"D:\OS-QEMU-EMU\qemu\qemu-system-x86_64.exe"
UEFI_BIOS = r"D:\OS-QEMU-EMU\qemu\share\edk2-x86_64-code.fd"
LOG_FILE = r"build\qemu_pxe_test.log"

if os.path.exists(LOG_FILE):
    try: os.remove(LOG_FILE)
    except: pass

cmd = [
    QEMU_EXE,
    "-drive", f"if=pflash,format=raw,readonly=on,file={UEFI_BIOS}",
    "-netdev", "user,id=net0,tftp=build,bootfile=BOOTX64.EFI",
    "-device", "virtio-net-pci,netdev=net0",
    "-boot", "n",
    "-m", "2048M",
    "-serial", f"file:{LOG_FILE}",
    "-display", "none",
    "-no-reboot"
]

print("[TEST QEMU PXE] Launching pure UEFI network boot test...")
proc = subprocess.Popen(cmd)

try:
    for i in range(35):
        time.sleep(1)
        if proc.poll() is not None:
            break
finally:
    if proc.poll() is None:
        proc.terminate()
        try: proc.wait(timeout=3)
        except: proc.kill()

print(f"[TEST QEMU PXE] QEMU exited with code {proc.poll()}")
if os.path.exists(LOG_FILE):
    with open(LOG_FILE, "r", errors="ignore") as f:
        lines = f.readlines()
    print(f"[TEST QEMU PXE] Captured {len(lines)} serial log lines:")
    for l in lines[:60]:
        print(l, end="")
    if len(lines) > 60:
        print("...\n" + "".join(lines[-30:]))
