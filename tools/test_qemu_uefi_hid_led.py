#!/usr/bin/env python3
"""
ATOMS OS — QEMU UEFI USB HID KEYBOARD LED VALIDATION SUITE
Tests pure UEFI boot with OVMF + qemu-xhci + usb-kbd,
injects keypresses via QMP, verifies ATOMS OS lock state transitions,
SET_REPORT dispatch, and xHCI completion.
"""

import sys
import os
import time
import socket
import json
import subprocess

QEMU_EXE = r"D:\OS-QEMU-EMU\qemu\qemu-system-x86_64.exe"
OVMF_BIOS = r"D:\OS-QEMU-EMU\qemu\share\edk2-x86_64-code.fd"
DISK_IMG = r"build\atoms_uefi_test.img"
SERIAL_LOG = r"build\qemu_uefi_serial.log"
SCREENSHOT_PPM = r"build\qemu_uefi_screen.ppm"
QMP_PORT = 4444

class QmpClient:
    def __init__(self, port=QMP_PORT):
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.settimeout(10.0)
        self.port = port
        self.buf = ""

    def connect(self, retries=15):
        for i in range(retries):
            try:
                self.sock.connect(("127.0.0.1", self.port))
                while "\r\n" not in self.buf:
                    chunk = self.sock.recv(4096).decode("utf-8", errors="ignore")
                    self.buf += chunk
                self.buf = "" # Clear greeting
                resp = self.execute({"execute": "qmp_capabilities"})
                print(f"[QMP] Handshake successful: {resp}")
                return True
            except Exception as e:
                time.sleep(1.0)
        print(f"[QMP] Failed to connect after {retries} retries.")
        return False

    def execute(self, cmd_dict):
        msg = json.dumps(cmd_dict) + "\r\n"
        self.sock.sendall(msg.encode("utf-8"))
        while True:
            while "\r\n" in self.buf:
                line, self.buf = self.buf.split("\r\n", 1)
                line = line.strip()
                if not line:
                    continue
                try:
                    obj = json.loads(line)
                    if "return" in obj or "error" in obj:
                        return obj
                except Exception:
                    pass
            chunk = self.sock.recv(4096).decode("utf-8", errors="ignore")
            if not chunk:
                return {}
            self.buf += chunk

    def send_key(self, key_name, hold_ms=250):
        return self.execute({
            "execute": "send-key",
            "arguments": {
                "keys": [{"type": "qcode", "data": key_name}],
                "hold-time": hold_ms
            }
        })

    def screendump(self, path):
        return self.execute({
            "execute": "screendump",
            "arguments": {"filename": os.path.abspath(path)}
        })

    def close(self):
        try:
            self.sock.close()
        except Exception:
            pass

def wait_for_event(log_pos, expected_reason, expected_mask, timeout=6.0):
    start = time.time()
    while time.time() - start < timeout:
        if os.path.exists(SERIAL_LOG):
            with open(SERIAL_LOG, "rb") as f:
                f.seek(log_pos)
                chunk = f.read().decode(errors="ignore")
                if expected_reason in chunk and f"Mask={expected_mask}" in chunk:
                    return True, chunk, f.tell()
        time.sleep(0.1)
    with open(SERIAL_LOG, "rb") as f:
        f.seek(log_pos)
        chunk = f.read().decode(errors="ignore")
    return False, chunk, os.path.getsize(SERIAL_LOG)

def main():
    if os.path.exists(SERIAL_LOG):
        try: os.remove(SERIAL_LOG)
        except Exception: pass

    cmd = [
        QEMU_EXE,
        "-drive", f"if=pflash,format=raw,readonly=on,file={OVMF_BIOS}",
        "-drive", f"file={DISK_IMG},format=raw",
        "-device", "qemu-xhci",
        "-device", "usb-kbd",
        "-device", "usb-mouse",
        "-m", "2048M",
        "-qmp", f"tcp:127.0.0.1:{QMP_PORT},server,nowait",
        "-serial", f"file:{SERIAL_LOG}",
        "-display", "none",
        "-no-reboot"
    ]

    print("[QEMU-TEST] Launching QEMU in Pure UEFI Mode with xHCI & usb-kbd...")
    proc = subprocess.Popen(cmd)

    try:
        qmp = QmpClient(QMP_PORT)
        if not qmp.connect():
            print("[QEMU-TEST] ERROR: Could not connect to QMP server.")
            proc.kill()
            return 1

        print("[QEMU-TEST] Waiting for ATOMS OS to complete UEFI boot & enter interactive loop (25s)...")
        booted = False
        for sec in range(25):
            time.sleep(1.0)
            if os.path.exists(SERIAL_LOG):
                with open(SERIAL_LOG, "rb") as f:
                    content = f.read().decode(errors="ignore")
                    if "Entering live interactive loop" in content:
                        print(f"[QEMU-TEST] Boot completed in {sec+1}s!")
                        booted = True
                        break

        if not booted:
            print("[QEMU-TEST] ERROR: Boot timeout!")
            return 1

        time.sleep(1.0)

        # -------------------------------------------------------------
        # STEP 1: VERIFY INITIAL BOOT STATE (Num=OFF, Caps=OFF, Scroll=OFF)
        # -------------------------------------------------------------
        print("\n=========================================================")
        print(" [TEST 1] VERIFYING FRESH BOOT INITIAL LED STATE")
        print("=========================================================")
        with open(SERIAL_LOG, "rb") as f:
            log_text = f.read().decode(errors="ignore")

        init_dispatched = ("Mask=0x00" in log_text and "BOOT_INIT" in log_text)
        print(f"[TEST 1] Initial LED State Dispatched: {'PASS (Mask=0x00 All-OFF)' if init_dispatched else 'FAIL'}")

        current_log_pos = os.path.getsize(SERIAL_LOG)

        # -------------------------------------------------------------
        # STEP 2: CAPS LOCK TOGGLE TEST (OFF -> ON -> OFF)
        # -------------------------------------------------------------
        print("\n=========================================================")
        print(" [TEST 2] MANUAL CAPS LOCK SINGLE-TOGGLE TEST")
        print("=========================================================")
        
        print("[TEST 2.1] Injecting physical CAPS_LOCK press (OFF -> ON)...")
        qmp.send_key("caps_lock")
        caps_on_pass, chunk, current_log_pos = wait_for_event(current_log_pos, "CAPS_CHANGE", "0x02")
        print(f"[SERIAL CHUNK]:\n{chunk.strip()}")
        print(f"[TEST 2.1] Caps OFF -> ON Result: {'PASS' if caps_on_pass else 'FAIL'}")

        time.sleep(0.5)
        print("\n[TEST 2.2] Injecting second physical CAPS_LOCK press (ON -> OFF)...")
        qmp.send_key("caps_lock")
        caps_off_pass, chunk, current_log_pos = wait_for_event(current_log_pos, "CAPS_CHANGE", "0x00")
        print(f"[SERIAL CHUNK]:\n{chunk.strip()}")
        print(f"[TEST 2.2] Caps ON -> OFF Result: {'PASS' if caps_off_pass else 'FAIL'}")

        # -------------------------------------------------------------
        # STEP 3: NUM LOCK TOGGLE TEST (OFF -> ON -> OFF)
        # -------------------------------------------------------------
        print("\n=========================================================")
        print(" [TEST 3] MANUAL NUM LOCK SINGLE-TOGGLE TEST")
        print("=========================================================")
        
        time.sleep(0.5)
        print("[TEST 3.1] Injecting physical NUM_LOCK press (OFF -> ON)...")
        qmp.send_key("num_lock")
        num_on_pass, chunk, current_log_pos = wait_for_event(current_log_pos, "NUM_CHANGE", "0x01")
        print(f"[SERIAL CHUNK]:\n{chunk.strip()}")
        print(f"[TEST 3.1] Num OFF -> ON Result: {'PASS' if num_on_pass else 'FAIL'}")

        time.sleep(0.5)
        print("\n[TEST 3.2] Injecting second physical NUM_LOCK press (ON -> OFF)...")
        qmp.send_key("num_lock")
        num_off_pass, chunk, current_log_pos = wait_for_event(current_log_pos, "NUM_CHANGE", "0x00")
        print(f"[SERIAL CHUNK]:\n{chunk.strip()}")
        print(f"[TEST 3.2] Num ON -> OFF Result: {'PASS' if num_off_pass else 'FAIL'}")

        # -------------------------------------------------------------
        # STEP 4: COMBINED 4-STATE MATRIX TEST
        # -------------------------------------------------------------
        print("\n=========================================================")
        print(" [TEST 4] COMBINED 4-STATE MATRIX TEST")
        print("=========================================================")
        
        # State 1: Num ON + Caps ON -> Mask 0x03
        time.sleep(0.5)
        print("[TEST 4.1] Setting Num ON + Caps ON...")
        qmp.send_key("num_lock")
        wait_for_event(current_log_pos, "NUM_CHANGE", "0x01", timeout=2.0)
        current_log_pos = os.path.getsize(SERIAL_LOG)
        time.sleep(0.3)
        qmp.send_key("caps_lock")
        s1_pass, chunk, current_log_pos = wait_for_event(current_log_pos, "CAPS_CHANGE", "0x03")
        print(f"[TEST 4.1] Num=ON Caps=ON -> Mask 0x03: {'PASS' if s1_pass else 'FAIL'}")

        # State 2: Num OFF + Caps ON -> Mask 0x02
        time.sleep(0.5)
        print("[TEST 4.2] Setting Num OFF + Caps ON...")
        qmp.send_key("num_lock")
        s2_pass, chunk, current_log_pos = wait_for_event(current_log_pos, "NUM_CHANGE", "0x02")
        print(f"[TEST 4.2] Num=OFF Caps=ON -> Mask 0x02: {'PASS' if s2_pass else 'FAIL'}")

        # State 3: Num ON + Caps OFF -> Mask 0x01
        time.sleep(0.5)
        print("[TEST 4.3] Setting Num ON + Caps OFF...")
        qmp.send_key("caps_lock")
        wait_for_event(current_log_pos, "CAPS_CHANGE", "0x00", timeout=2.0)
        current_log_pos = os.path.getsize(SERIAL_LOG)
        time.sleep(0.3)
        qmp.send_key("num_lock")
        s3_pass, chunk, current_log_pos = wait_for_event(current_log_pos, "NUM_CHANGE", "0x01")
        print(f"[TEST 4.3] Num=ON Caps=OFF -> Mask 0x01: {'PASS' if s3_pass else 'FAIL'}")

        # State 4: Num OFF + Caps OFF -> Mask 0x00
        time.sleep(0.5)
        print("[TEST 4.4] Setting Num OFF + Caps OFF...")
        qmp.send_key("num_lock")
        s4_pass, chunk, current_log_pos = wait_for_event(current_log_pos, "NUM_CHANGE", "0x00")
        print(f"[TEST 4.4] Num=OFF Caps=OFF -> Mask 0x00: {'PASS' if s4_pass else 'FAIL'}")

        # Capture Screendump
        print("\n[QEMU-TEST] Capturing QEMU screendump...")
        qmp.screendump(SCREENSHOT_PPM)
        print(f"[QEMU-TEST] Screendump saved to {SCREENSHOT_PPM}")

        # Summary
        all_passed = (init_dispatched and caps_on_pass and caps_off_pass and num_on_pass and num_off_pass and s1_pass and s2_pass and s3_pass and s4_pass)
        print("\n=========================================================")
        print(f" [QEMU UEFI VALIDATION SUMMARY] : {'ALL TESTS PASSED (100%)' if all_passed else 'TESTS FAILED'}")
        print("=========================================================")
        return 0 if all_passed else 2

    finally:
        qmp.close()
        proc.kill()

if __name__ == "__main__":
    sys.exit(main())
