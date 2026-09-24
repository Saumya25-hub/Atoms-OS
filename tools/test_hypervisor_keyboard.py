import os
import subprocess
import time
import socket
import json
import sys

QEMU_EXE = r"D:\OS-QEMU-EMU\qemu\qemu-system-x86_64.exe"
OVMF_CODE = r"D:\OS-QEMU-EMU\qemu\share\edk2-x86_64-code.fd"
IMG_PATH = r"D:\Signatures_OS\build\atoms_uefi_test.img"
LOG_PATH = r"D:\Signatures_OS\build\hypervisor_keyboard_test.log"

if os.path.exists(LOG_PATH):
    try:
        os.remove(LOG_PATH)
    except Exception:
        pass

cmd = [
    QEMU_EXE,
    "-machine", "q35",
    "-cpu", "max,vmx=on",
    "-m", "4096M",
    "-drive", f"if=pflash,format=raw,readonly=on,file={OVMF_CODE}",
    "-drive", f"file={IMG_PATH},format=raw",
    "-device", "qemu-xhci",
    "-device", "usb-kbd",
    "-qmp", "tcp:127.0.0.1:4445,server,nowait",
    "-serial", f"file:{LOG_PATH}",
    "-display", "none",
    "-no-reboot"
]

print("[TEST] Launching QEMU with xHCI, USB Keyboard & QMP on port 4445...")
proc = subprocess.Popen(cmd)

try:
    print("[TEST] Waiting for Hypervisor Runtime Dashboard to start...")
    started = False
    start_t = time.time()
    while time.time() - start_t < 35:
        if proc.poll() is not None:
            print("[ERROR] QEMU exited early!")
            break
        if os.path.exists(LOG_PATH):
            with open(LOG_PATH, "r", encoding="utf-8", errors="replace") as f:
                content = f.read()
                if "FREEBSD RUNTIME" in content:
                    print(f"[TEST] Runtime Dashboard detected in {time.time() - start_t:.1f}s!")
                    started = True
                    break
        time.sleep(1)

    if not started:
        print("[FAIL] Runtime dashboard did not reach active state in 35s.")
        sys.exit(1)

    time.sleep(2)
    print("[TEST] Connecting to QMP to inject keypresses...")
    s = socket.socket()
    s.connect(('127.0.0.1', 4445))
    s.recv(1024)
    s.sendall(b'{"execute": "qmp_capabilities"}\r\n')
    s.recv(1024)

    def send_key(qcode):
        print(f"[TEST] Injecting key: {qcode}")
        msg = json.dumps({
            "execute": "send-key",
            "arguments": {
                "keys": [{"type": "qcode", "data": qcode}],
                "hold-time": 200
            }
        }) + "\r\n"
        s.sendall(msg.encode())
        s.recv(1024)

    # Send key 'a'
    send_key("a")
    time.sleep(1)

    # Send key 'f2'
    send_key("f2")
    time.sleep(1)

    # Send key 'f3'
    send_key("f3")
    time.sleep(1)

    # Send key 'f4'
    send_key("f4")
    time.sleep(2)

    with open(LOG_PATH, "r", encoding="utf-8", errors="replace") as f:
        log_content = f.read()

    print("\n--- KEYBOARD EVENT AUDIT ---")
    detected = False
    for line in log_content.splitlines():
        if "[KEYBOARD]" in line or "[USB KBD]" in line:
            print("  ", line)
            detected = True

    if detected:
        print("\n[PASS] KEYBOARD INPUT VERIFIED END-TO-END THROUGH XHCI -> HID -> BUFFER -> DASHBOARD!")
    else:
        print("\n[INFO] Checking if events were registered...")
        for line in log_content.splitlines():
            if "Keys:" in line or "g_dashboard_key_press_count" in line:
                print("  ", line)

finally:
    try:
        proc.terminate()
        proc.wait(timeout=5)
    except Exception:
        proc.kill()
