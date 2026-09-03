import time
import subprocess
import socket
import json
import os

LOG_FILE = r"build\test_keys.log"
if os.path.exists(LOG_FILE):
    os.remove(LOG_FILE)

proc = subprocess.Popen([
    r"D:\OS-QEMU-EMU\qemu\qemu-system-x86_64.exe",
    "-drive", r"if=pflash,format=raw,readonly=on,file=D:\OS-QEMU-EMU\qemu\share\edk2-x86_64-code.fd",
    "-drive", r"file=build\atoms_uefi_test.img,format=raw",
    "-device", "qemu-xhci",
    "-device", "usb-kbd",
    "-m", "2048M",
    "-qmp", "tcp:127.0.0.1:4444,server,nowait",
    "-serial", f"file:{LOG_FILE}",
    "-display", "none"
])

print("Waiting for ATOMS OS to reach live interactive loop...")
for i in range(30):
    time.sleep(1)
    if os.path.exists(LOG_FILE):
        with open(LOG_FILE, "r", errors="ignore") as f:
            if "Entering live interactive loop" in f.read():
                print(f"Boot reached interactive loop in {i+1}s!")
                break

time.sleep(3)

s = socket.socket()
s.connect(('127.0.0.1', 4444))
s.recv(1024)
s.sendall(b'{"execute": "qmp_capabilities"}\r\n')
s.recv(1024)

def send_key(k, hold=250):
    print(f"Injecting {k} (hold={hold}ms)...")
    msg = json.dumps({
        "execute": "send-key",
        "arguments": {
            "keys": [{"type": "qcode", "data": k}],
            "hold-time": hold
        }
    }) + "\r\n"
    s.sendall(msg.encode())
    s.recv(1024)

# 1. CapsLock ON
send_key("caps_lock", 250)
time.sleep(3.0)

# 2. CapsLock OFF
send_key("caps_lock", 250)
time.sleep(3.0)

# 3. NumLock ON
send_key("num_lock", 250)
time.sleep(3.0)

# 4. NumLock OFF
send_key("num_lock", 250)
time.sleep(3.0)

s.close()
proc.kill()

with open(LOG_FILE, "r", errors="ignore") as f:
    lines = f.readlines()
    print("========================================")
    print("--- LAST 40 LINES OF SERIAL LOG ---")
    print("========================================")
    for l in lines[-40:]:
        print(l.strip())
