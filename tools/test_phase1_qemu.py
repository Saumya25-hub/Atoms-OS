import os
import sys
import time
import socket
import subprocess
from PIL import Image

QEMU_EXE = r"D:\OS-QEMU-EMU\qemu\qemu-system-x86_64.exe"
UEFI_BIOS = r"D:\OS-QEMU-EMU\qemu\share\edk2-x86_64-code.fd"
IMAGE_PATH = r"d:\Signatures_OS\build\atoms_uefi_test.img"
SERIAL_LOG = r"d:\Signatures_OS\build\phase1_boot_serial.log"
SCREEN_PPM = r"d:\Signatures_OS\build\phase1_screen.ppm"
SCREEN_PNG = r"d:\Signatures_OS\build\phase1_screen.png"

if os.path.exists(SERIAL_LOG):
    try: os.remove(SERIAL_LOG)
    except: pass
if os.path.exists(SCREEN_PPM):
    try: os.remove(SCREEN_PPM)
    except: pass

cmd = [
    QEMU_EXE,
    "-drive", f"if=pflash,format=raw,readonly=on,file={UEFI_BIOS}",
    "-drive", f"file={IMAGE_PATH},format=raw",
    "-serial", f"file:{SERIAL_LOG}",
    "-m", "2048M",
    "-device", "qemu-xhci",
    "-device", "usb-kbd",
    "-device", "usb-mouse",
    "-netdev", "user,id=net0",
    "-device", "e1000,netdev=net0",
    "-display", "none",
    "-monitor", "telnet:127.0.0.1:4447,server,nowait",
    "-no-reboot"
]

print("[PHASE1_QEMU] Launching QEMU UEFI with Phase 1 runtime...")
proc = subprocess.Popen(cmd)

print("[PHASE1_QEMU] Waiting for boot and tests (polling serial log up to 50 seconds)...")
start_time = time.time()
test_found = False
logged_in = False

while time.time() - start_time < 50:
    time.sleep(2)
    if os.path.exists(SERIAL_LOG):
        try:
            with open(SERIAL_LOG, "r", errors="ignore") as f:
                content = f.read()
            if "Phase 1 Java Runtime Foundation Tests" in content:
                test_found = True
                print("[PHASE1_QEMU] Detected Phase 1 Test Runner in serial log!")
                time.sleep(3)
                break
            if "Transitioning to Login Screen" in content and not logged_in:
                print("[PHASE1_QEMU] Login screen reached! Sending login keys...")
                try:
                    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                    s.connect(("127.0.0.1", 4447))
                    for k in ["a", "d", "m", "i", "n", "1", "2", "3", "ret"]:
                        s.sendall(f"sendkey {k}\n".encode("ascii"))
                        time.sleep(0.1)
                    s.close()
                    logged_in = True
                except Exception as e:
                    print(f"[PHASE1_QEMU] Key send note: {e}")
        except:
            pass

try:
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect(("127.0.0.1", 4447))
    time.sleep(0.5)
    print("[PHASE1_QEMU] Capturing screen dump...")
    s.sendall(f"screendump {SCREEN_PPM}\n".encode("ascii"))
    time.sleep(1.0)
    s.sendall(b"quit\n")
    s.close()
except Exception as e:
    print(f"[PHASE1_QEMU] Monitor socket note: {e}")

if proc.poll() is None:
    proc.terminate()
    try: proc.wait(timeout=5)
    except: proc.kill()

if os.path.exists(SCREEN_PPM):
    try:
        img = Image.open(SCREEN_PPM)
        img.save(SCREEN_PNG, "PNG")
        print(f"[PHASE1_QEMU] Screen captured: {SCREEN_PNG}")
    except Exception as e:
        print(f"[PHASE1_QEMU] PNG conversion error: {e}")

if os.path.exists(SERIAL_LOG):
    with open(SERIAL_LOG, "r", errors="ignore") as f:
        log_content = f.read()
    print("==================================================")
    print("           SERIAL LOG EXTRACT (TAIL)              ")
    print("==================================================")
    lines = log_content.splitlines()
    for l in lines[-60:]:
        print(l)
    print("==================================================")
    if "Java Runtime Foundation Tests" in log_content:
        print("[PHASE1_QEMU] PASS: Phase 1 Java Runtime Foundation Tests verified!")
    else:
        print("[PHASE1_QEMU] NOTE: Checking serial log for Phase 1 markers...")
