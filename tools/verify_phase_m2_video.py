#!/usr/bin/env python3
"""
ATOMS OS — Phase M2 Real MP4 Video Playback Verification
Tests pure UEFI boot with OVMF + atoms_uefi_test.img.
Automates ROOK lock screen dismissal & authentication via QMP.
Verifies ISO BMFF demuxer, H.264 slice decoder, and BOSurface v2.5 frame presentation.
"""

import os
import sys
import time
import socket
import json
import subprocess
import shutil

QEMU_EXE = r"D:\OS-QEMU-EMU\qemu\qemu-system-x86_64.exe"
if not os.path.exists(QEMU_EXE):
    QEMU_EXE = shutil.which("qemu-system-x86_64") or "qemu-system-x86_64"

OVMF_BIOS = r"D:\OS-QEMU-EMU\qemu\share\edk2-x86_64-code.fd"
GPT_IMG = r"build\atoms_uefi_test.img"
SERIAL_LOG = r"build\m2_video_serial.log"
QMP_PORT = 4466

EXPECTED_TELEMETRY = [
    "[MP4] file opened",
    "[MP4] moov parsed",
    "[MP4] video track found",
    "[MP4] codec = avc1",
    "[MP4] resolution = 1280x720",
    "[MP4] mdat sample extraction = OK",
    "[H264] SPS = OK",
    "[H264] PPS = OK",
    "[H264] decoder initialized",
    "[VIDEO] frame 0 decoded",
    "[BOSURFACE] frame presented",
    "[VIDEO] playback started"
]

def send_qmp_key(sock, key, hold=150):
    msg = json.dumps({
        "execute": "send-key",
        "arguments": {
            "keys": [{"type": "qcode", "data": key}],
            "hold-time": hold
        }
    }) + "\r\n"
    sock.sendall(msg.encode())
    try:
        sock.recv(1024)
    except Exception:
        pass
    time.sleep(0.3)

def main():
    print("========================================================")
    print("  ATOMS OS — PHASE M2 REAL MP4 VIDEO PLAYBACK TEST")
    print("========================================================")

    if os.path.exists(SERIAL_LOG):
        try:
            os.remove(SERIAL_LOG)
        except OSError:
            pass

    cmd = [
        QEMU_EXE,
        "-drive", f"if=pflash,format=raw,readonly=on,file={OVMF_BIOS}",
        "-drive", f"file={GPT_IMG},format=raw",
        "-device", "qemu-xhci",
        "-device", "usb-mouse",
        "-device", "usb-kbd",
        "-qmp", f"tcp:127.0.0.1:{QMP_PORT},server,nowait",
        "-serial", f"file:{SERIAL_LOG}",
        "-m", "2048M",
        "-display", "none",
        "-no-reboot"
    ]

    print(f"[QEMU] Command: {' '.join(cmd)}")
    proc = subprocess.Popen(cmd)

    print("[QEMU] Waiting for Login Supervisor Loop in serial log...")
    reached_login = False
    for i in range(40):
        time.sleep(1)
        if os.path.exists(SERIAL_LOG):
            try:
                with open(SERIAL_LOG, "r", encoding="utf-8", errors="ignore") as f:
                    content = f.read()
                if "Entering Interactive Login Supervisor Loop" in content:
                    print(f"[QEMU] Reached login loop in {i+1}s!")
                    reached_login = True
                    break
            except Exception:
                pass
        if proc.poll() is not None:
            print(f"[QEMU] Exited early with code {proc.poll()}")
            break

    if reached_login:
        time.sleep(1.0)
        try:
            s = socket.socket()
            s.connect(('127.0.0.1', QMP_PORT))
            s.recv(1024)
            s.sendall(b'{"execute": "qmp_capabilities"}\r\n')
            s.recv(1024)

            print("[QEMU] Dismissing lock screen (Space)...")
            send_qmp_key(s, "spc")
            time.sleep(3.5)

            print("[QEMU] Typing password 'admin123'...")
            for ch in ["a", "d", "m", "i", "n", "1", "2", "3"]:
                send_qmp_key(s, ch)

            time.sleep(0.5)
            print("[QEMU] Pressing Enter to log in...")
            send_qmp_key(s, "ret")

            s.close()
        except Exception as e:
            print(f"[QEMU] QMP automation exception: {e}")

    print("[QEMU] Waiting for Desktop session and Media Player playback...")
    start_time = time.time()
    for sec in range(40):
        time.sleep(1)
        if os.path.exists(SERIAL_LOG):
            try:
                with open(SERIAL_LOG, "r", encoding="utf-8", errors="ignore") as f:
                    content = f.read()
                missing = [m for m in EXPECTED_TELEMETRY if m not in content]
                if not missing:
                    print(f"[QEMU] All {len(EXPECTED_TELEMETRY)} required marks detected after {int(time.time() - start_time)}s!")
                    break
            except Exception:
                pass
        if proc.poll() is not None:
            break

    try:
        proc.terminate()
        proc.wait(timeout=3)
    except Exception:
        try:
            proc.kill()
        except Exception:
            pass

    print(f"\n--- Serial Output Telemetry Analysis ({SERIAL_LOG}) ---")
    if os.path.exists(SERIAL_LOG):
        with open(SERIAL_LOG, "r", encoding="utf-8", errors="ignore") as f:
            lines = f.readlines()
        
        for line in lines:
            if any(k in line for k in ["[MP4]", "[H264]", "[VIDEO]", "[BOSURFACE]", "[BOSPECTRA]", "[AUDIO]", "TRACE"]):
                safe_line = line.strip().encode("ascii", errors="replace").decode("ascii")
                print("  " + safe_line)

        full_content = "".join(lines)
        results = {}
        for mark in EXPECTED_TELEMETRY:
            results[mark] = (mark in full_content)

        print("\n--- Forensic Checklist ---")
        all_ok = True
        for mark, ok in results.items():
            status = "PASS" if ok else "FAIL"
            if not ok:
                all_ok = False
            print(f"  [{status}] {mark}")

        if all_ok:
            print("\n>>> OVERALL VERDICT: PASS <<<")
            return 0
        else:
            print("\n>>> OVERALL VERDICT: FAIL (Some telemetry marks missing) <<<")
            return 1
    else:
        print("[ERROR] Serial log was not created!")
        return 2

if __name__ == "__main__":
    sys.exit(main())
