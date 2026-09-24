import os
import subprocess
import time
import sys

QEMU_CANDIDATES = [
    r"D:\OS-QEMU-EMU\qemu\qemu-system-x86_64.exe",
    r"C:\Program Files\qemu\qemu-system-x86_64.exe",
    "qemu-system-x86_64"
]

OVMF_CANDIDATES = [
    r"D:\OS-QEMU-EMU\qemu\share\edk2-x86_64-code.fd",
    r"C:\Program Files\qemu\share\edk2-x86_64-code.fd",
    r"D:\Signatures_OS\tools\ovmf\OVMF.fd",
]

IMG_PATH = r"D:\Signatures_OS\build\atoms_uefi_test.img"
LOG_PATH = r"D:\Signatures_OS\build\freebsd_hypervisor_serial.log"

def get_qemu():
    for c in QEMU_CANDIDATES:
        if os.path.exists(c):
            return c
    return "qemu-system-x86_64"

def get_ovmf():
    for c in OVMF_CANDIDATES:
        if os.path.exists(c):
            return c
    return None

def main():
    qemu_exe = get_qemu()
    ovmf_code = get_ovmf()

    if not os.path.exists(IMG_PATH):
        print(f"[ERROR] OS image not found at {IMG_PATH}")
        return False

    if os.path.exists(LOG_PATH):
        try:
            os.remove(LOG_PATH)
        except Exception:
            pass

    cmd = [
        qemu_exe,
        "-machine", "q35",
        "-cpu", "max,vmx=on",
        "-m", "4096M",
    ]
    if ovmf_code:
        cmd.extend(["-drive", f"if=pflash,format=raw,readonly=on,file={ovmf_code}"])
    cmd.extend([
        "-drive", f"file={IMG_PATH},format=raw",
        "-device", "qemu-xhci",
        "-device", "usb-kbd",
        "-device", "usb-mouse",
        "-netdev", "user,id=net0",
        "-device", "e1000,netdev=net0",
        "-serial", f"file:{LOG_PATH}",
        "-display", "none",
        "-no-reboot"
    ])

    print("[1/3] Launching QEMU in UEFI mode with nested VMX enabled...")
    print("  Command:", " ".join(cmd))
    proc = subprocess.Popen(cmd)

    print("[2/3] Streaming live serial COM1 output for 90 seconds...")
    start_t = time.time()
    last_pos = 0
    while time.time() - start_t < 90:
        if proc.poll() is not None:
            break
        if os.path.exists(LOG_PATH):
            with open(LOG_PATH, "r", encoding="utf-8", errors="replace") as f:
                f.seek(last_pos)
                new_data = f.read()
                if new_data:
                    sys.stdout.buffer.write(new_data.encode('utf-8', errors='replace'))
                    sys.stdout.buffer.flush()
                    last_pos = f.tell()
        time.sleep(0.5)

    if proc.poll() is None:
        proc.terminate()
        try:
            proc.wait(timeout=3)
        except Exception:
            proc.kill()

    print("\n[3/3] Final Serial Log Verification...")
    if os.path.exists(LOG_PATH):
        with open(LOG_PATH, "r", encoding="utf-8", errors="replace") as f:
            content = f.read()
            print(f"  Total captured bytes: {len(content)}")
            print("--- LOG DUMP START ---")
            dump_text = content[-2000:] if len(content) > 2000 else content
            sys.stdout.buffer.write(dump_text.encode('utf-8', errors='replace'))
            sys.stdout.buffer.flush()
            print("\n--- LOG DUMP END ---")
            return True
    else:
        print("[ERROR] No serial log captured!")
        return False

if __name__ == "__main__":
    main()
