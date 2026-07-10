import subprocess
import time
import os
import sys

log_file = "serial_atm.log"
if os.path.exists(log_file):
    os.remove(log_file)

qemu_cmd = [
    r"D:\OS-QEMU-EMU\qemu\qemu-system-x86_64.exe",
    "-accel", "whpx",
    "-drive", r"file=build\SignaturesOS.vdi,format=vdi",
    "-m", "512M",
    "-device", "AC97",
    "-serial", f"file:{log_file}",
    "-display", "none"
]

print(f"Launching QEMU for 8 seconds...")
proc = subprocess.Popen(qemu_cmd)
time.sleep(8)
print("Terminating QEMU...")
proc.terminate()
try:
    proc.wait(timeout=3)
except subprocess.TimeoutExpired:
    proc.kill()

print("QEMU stopped. Reading serial log:")
if os.path.exists(log_file):
    with open(log_file, "r", errors="replace") as f:
        content = f.read()
    print("--- SERIAL LOG START ---")
    print(content)
    print("--- SERIAL LOG END ---")
else:
    print("ERROR: serial_atm.log not created!")
