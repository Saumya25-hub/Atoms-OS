#!/usr/bin/env python3
"""
ATOMS OS — ATOMS Bridge End-to-End Verification Harness
Tests QEMU UEFI boot, hypervisor runtime, and UDP command channel (port 9998).
"""

import os
import sys
import time
import subprocess
import socket

REPO_ROOT = r"D:\Signatures_OS"
IMG_PATH = os.path.join(REPO_ROOT, "build", "atoms_uefi_test.img")
LOG_PATH = os.path.join(REPO_ROOT, "build", "bridge_e2e_serial.log")

QEMU_EXE = r"D:\OS-QEMU-EMU\qemu\qemu-system-x86_64.exe"
OVMF_CODE = r"D:\OS-QEMU-EMU\qemu\share\edk2-x86_64-code.fd"

if not os.path.exists(QEMU_EXE):
    QEMU_EXE = r"C:\Program Files\qemu\qemu-system-x86_64.exe"
if not os.path.exists(OVMF_CODE):
    OVMF_CODE = r"C:\Program Files\qemu\share\edk2-x86_64-code.fd"

def run_bridge_cmd(cmd_str, target_ip="127.0.0.1", port=9998, timeout=2.0, multi_packet=False, retries=3):
    for attempt in range(retries):
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.settimeout(timeout)
        sock.sendto((cmd_str.strip() + "\n").encode("utf-8"), (target_ip, port))
        chunks = []
        expected_rips = None
        total_rips = 0

        try:
            while True:
                try:
                    data, _ = sock.recvfrom(2048)
                    text = data.decode("utf-8", errors="replace")
                    chunks.append(text)
                    if multi_packet:
                        for line in text.splitlines():
                            if line.startswith("rip_count="):
                                try:
                                    expected_rips = int(line.split("=")[1])
                                except ValueError:
                                    pass
                            elif line.startswith("0x"):
                                total_rips += 1
                        if expected_rips is not None and total_rips >= expected_rips:
                            break
                        sock.settimeout(0.5)
                    else:
                        break
                except socket.timeout:
                    break
        finally:
            sock.close()
        res = "".join(chunks)
        if res:
            return res
        time.sleep(0.3)
    return ""

def main():
    print("=" * 65)
    print(" ATOMS BRIDGE END-TO-END VERIFICATION HARNESS")
    print("=" * 65)

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
        "-device", "usb-mouse",
        "-netdev", "user,id=net0,hostfwd=udp:127.0.0.1:9998-10.0.2.15:9998,hostfwd=udp:127.0.0.1:9999-10.0.2.15:9999",
        "-device", "e1000,netdev=net0",
        "-serial", f"file:{LOG_PATH}",
        "-display", "none",
        "-no-reboot"
    ]

    print(f"[1/5] Launching QEMU with nested VMX and port forward (UDP 9998)...")
    proc = subprocess.Popen(cmd)

    bridge_ready = False
    runtime_active = False
    start_t = time.time()
    last_pos = 0

    print(f"[2/5] Waiting for ATOMS Bridge initialization & guest runtime...")
    while time.time() - start_t < 90:
        if proc.poll() is not None:
            print(f"[ERROR] QEMU exited prematurely with code {proc.returncode}!")
            break

        if os.path.exists(LOG_PATH):
            with open(LOG_PATH, "r", encoding="utf-8", errors="replace") as f:
                f.seek(last_pos)
                new_text = f.read()
                if new_text:
                    if "[BRIDGE] ATOMS Bridge active" in new_text:
                        bridge_ready = True
                    if "PERSISTENT GUEST ACTIVE" in new_text or "PERSISTENT REAL RUNTIME" in new_text:
                        runtime_active = True
                    # Echo serial
                    for line in new_text.splitlines():
                        if any(k in line for k in ("BRIDGE", "GUEST", "RUNTIME", "VMEXIT", "TEST", "DHCP", "NET")):
                            print(f"  [SERIAL] {line.strip()}")
                    last_pos = f.tell()

        if bridge_ready and runtime_active:
            print(f"[SUCCESS] Hypervisor runtime active and bridge registered on UDP 9998!")
            time.sleep(1.0)
            break
        time.sleep(0.5)

    if not (bridge_ready and runtime_active):
        print(f"[WARN] Bridge/Runtime detection incomplete within timeout. Attempting probes anyway...")

    print("\n" + "=" * 65)
    print(" [3/5] TESTING BRIDGE UDP COMMANDS")
    print("=" * 65)

    # 1. State command
    print("\n---> COMMAND: state")
    resp_state = run_bridge_cmd("state")
    print(resp_state.strip() if resp_state else "[NO RESPONSE]")

    # 2. Memory command (GPA & GVA)
    print("\n---> COMMAND: mem gpa 0x1000 32")
    resp_mem_gpa = run_bridge_cmd("mem gpa 0x1000 32")
    print(resp_mem_gpa.strip() if resp_mem_gpa else "[NO RESPONSE]")

    print("\n---> COMMAND: mem gva 0xFFFFFFFF8038C395 16")
    resp_mem_gva = run_bridge_cmd("mem gva 0xFFFFFFFF8038C395 16")
    print(resp_mem_gva.strip() if resp_mem_gva else "[NO RESPONSE]")

    # 3. IDT command
    print("\n---> COMMAND: idt 49")
    resp_idt = run_bridge_cmd("idt 49")
    print(resp_idt.strip() if resp_idt else "[NO RESPONSE]")

    # 4. Rips command
    print("\n---> COMMAND: rips")
    resp_rips = run_bridge_cmd("rips", multi_packet=True)
    rips_lines = resp_rips.splitlines()
    print(f"Total lines received: {len(rips_lines)}")
    if rips_lines:
        print(f"First 5 lines: {rips_lines[:5]}")

    # 5. Type command
    print("\n---> COMMAND: type hello\\n")
    resp_type = run_bridge_cmd("type hello\\n")
    print(resp_type.strip() if resp_type else "[NO RESPONSE]")

    # Run bridge.py directly
    print("\n" + "=" * 65)
    print(" [4/5] TESTING tools/bridge.py with RIP Symbol Resolution")
    print("=" * 65)
    bridge_py = os.path.join(REPO_ROOT, "tools", "bridge.py")
    res = subprocess.run([sys.executable, bridge_py, "--ip", "127.0.0.1", "rips"], capture_output=True, text=True)
    print(res.stdout)
    if res.stderr:
        print("[STDERR]", res.stderr)

    # Teardown
    print("\n[5/5] Terminating test VM...")
    if proc.poll() is None:
        proc.terminate()
        try:
            proc.wait(timeout=3)
        except Exception:
            proc.kill()

    print("\nVerification run complete.")
    return 0

if __name__ == "__main__":
    sys.exit(main())
