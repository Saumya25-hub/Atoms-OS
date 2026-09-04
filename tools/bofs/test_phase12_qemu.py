import os
import sys
import time
import socket
import subprocess
from PIL import Image

QEMU_EXE   = r"D:\OS-QEMU-EMU\qemu\qemu-system-x86_64.exe"
UEFI_BIOS  = r"D:\OS-QEMU-EMU\qemu\share\edk2-x86_64-code.fd"
IMAGE_PATH = r"D:\Signatures_OS\build\atoms_uefi_test.img"
PPM_PATH   = r"D:\Signatures_OS\build\phase12_forensic_dashboard.ppm"
PNG_PATH   = r"D:\Signatures_OS\build\phase12_forensic_dashboard.png"
ARTIFACT_PNG = r"C:\Users\Saumya Chaudhari\.gemini\antigravity-ide\brain\cb7bd3da-a317-4942-a83f-2cc475b30df3\phase12_forensic_dashboard.png"
SERIAL_LOG = r"D:\Signatures_OS\build\phase12_forensic_serial.log"

for p in [PPM_PATH, PNG_PATH, SERIAL_LOG]:
    if os.path.exists(p):
        try:
            os.remove(p)
        except Exception:
            pass

cmd = [
    QEMU_EXE,
    "-drive", f"if=pflash,format=raw,readonly=on,file={UEFI_BIOS}",
    "-drive", f"file={IMAGE_PATH},format=raw",
    "-serial", f"file:{SERIAL_LOG}",
    "-smp", "4",
    "-m", "1024M",
    "-display", "none",
    "-monitor", "telnet:127.0.0.1:4456,server,nowait",
    "-no-reboot"
]

print("[TEST] Launching QEMU pure UEFI — BOFS Phase 12 Final Forensic Debug Dashboard...")
proc = subprocess.Popen(cmd)

print("[TEST] Monitoring serial log for BOFS Phase 12 completion...")
start_time = time.time()
test_completed = False

while time.time() - start_time < 60:
    time.sleep(1)
    if os.path.exists(SERIAL_LOG):
        try:
            with open(SERIAL_LOG, "r", errors="ignore") as f:
                log_data = f.read()
                if "[PHASE12] Phase 12 Forensic Debug Dashboard Complete: MASTER CERTIFICATION PASS" in log_data:
                    print(f"[TEST] BOFS Phase 12 completed successfully after {time.time() - start_time:.1f}s!")
                    test_completed = True
                    break
        except Exception:
            pass

time.sleep(2.0)

try:
    print("[TEST] Connecting to QEMU monitor on port 4456...")
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect(("127.0.0.1", 4456))
    time.sleep(1.0)

    print(f"[TEST] Capturing screendump to {PPM_PATH}...")
    s.sendall(f"screendump {PPM_PATH}\n".encode("ascii"))
    time.sleep(2.0)
    s.sendall(b"quit\n")
    s.close()
except Exception as e:
    print(f"[TEST] Monitor communication error: {e}")

time.sleep(1)
if proc.poll() is None:
    proc.terminate()

if os.path.exists(PPM_PATH):
    print(f"[TEST] Converting PPM to PNG: {PNG_PATH}...")
    img = Image.open(PPM_PATH)
    img.save(PNG_PATH, "PNG")
    try:
        img.save(ARTIFACT_PNG, "PNG")
        print(f"[TEST] SUCCESS: Saved {PNG_PATH} ({img.width}x{img.height}) and copied to artifacts.")
    except Exception as e:
        print(f"[TEST] Note on artifact save: {e}")
else:
    print("[TEST] Warning: PPM file was not created!")

if os.path.exists(SERIAL_LOG):
    print("\n================== COM1 SERIAL TELEMETRY LOG ==================")
    with open(SERIAL_LOG, "r", errors="ignore") as f:
        content = f.read()
        lines = content.splitlines()
        for line in lines[-70:]:
            print(line)
    print("===============================================================\n")

    if "MASTER CERTIFICATION PASS" in content:
        print("[TEST VERDICT] >>> PRE-FLIGHT VALIDATION: PASS <<<")
        sys.exit(0)
    else:
        print("[TEST VERDICT] >>> PRE-FLIGHT VALIDATION: FAILED <<<")
        sys.exit(1)
else:
    print("[TEST VERDICT] >>> PRE-FLIGHT VALIDATION: FAILED (No serial log) <<<")
    sys.exit(1)
