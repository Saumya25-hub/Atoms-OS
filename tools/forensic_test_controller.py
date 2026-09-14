#!/usr/bin/env python3
"""
ATOMS OS — Automated Forensic Test Controller & Autonomous Physical Test Supervisor
Target: Dedicated Physical TEST PC (ASUS B750M-K / Haswell H81 Testbed, MAC: A0:AD:9F:C5:81:27, IP: 192.168.2.100)
Host Controller: DEV LAPTOP (Ethernet IP: 192.168.2.1)
"""

import sys
import os
import socket
import struct
import time
import datetime
import argparse
import subprocess
import threading
import json
from collections import deque

try:
    sys.stdout.reconfigure(encoding='utf-8', errors='replace')
    sys.stderr.reconfigure(encoding='utf-8', errors='replace')
except Exception:
    pass

MAGIC_SPMS = 0x534D5053 # "SPMS" in little-endian
HEADER_FORMAT = "<IIHHIHH" # magic (I), session_id (I), total_chunks (H), chunk_index (H), offset (I), data_len (H), flags (H) = 20 bytes
HEADER_SIZE = struct.calcsize(HEADER_FORMAT)

SCREENSHOT_PORT = 9998
CONTROL_PORT = 9999
TARGET_MAC = "A0:AD:9F:C5:81:27"
TARGET_H81_IP = "192.168.2.100"
SERVER_IP = "192.168.2.1"

BASE_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUTPUT_DIR = os.path.join(BASE_DIR, "artifacts", "screenshots")
LOG_PATH = os.path.join(BASE_DIR, "artifacts", "forensic_action_ledger.log")
CASES_DIR = os.path.join(BASE_DIR, "artifacts", "cases")

def ensure_output_dir():
    if not os.path.exists(OUTPUT_DIR):
        os.makedirs(OUTPUT_DIR, exist_ok=True)
    os.makedirs(os.path.dirname(LOG_PATH), exist_ok=True)
    os.makedirs(CASES_DIR, exist_ok=True)

def log_action(action: str, target: str, reason: str, result: str):
    """
    Structured Forensic Action Logger
    Schema: timestamp | action | target | reason | result
    """
    ensure_output_dir()
    ts = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S.%f")[:-3]
    log_line = f"[{ts}] | ACTION: {action:<22} | TARGET: {target:<20} | REASON: {reason} | RESULT: {result}\n"
    with open(LOG_PATH, "a", encoding="utf-8") as f:
        f.write(log_line)
    print(f"[ACTION LEDGER] {action} -> {target} | {result}")

def send_control_command(cmd, target_ip=TARGET_H81_IP, port=CONTROL_PORT, reason="Manual operator command"):
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    msg = (cmd.strip() + "\n").encode('utf-8')
    try:
        sock.sendto(msg, (target_ip, port))
        sock.sendto(msg, ("192.168.2.255", port))
        log_action(f"LAN_{cmd}", f"{target_ip}:{port}", reason, "SENT_SUCCESS")
        print(f"[CONTROLLER] Sent '{cmd}' to {target_ip}:{port}")
    except Exception as e:
        log_action(f"LAN_{cmd}", f"{target_ip}:{port}", reason, f"ERROR: {e}")
        print(f"[CONTROLLER] Failed to send '{cmd}': {e}")
    finally:
        sock.close()

def send_reboot_command(target_ip=TARGET_H81_IP, reason="Automated physical test reboot"):
    """Sends UDP REBOOT command packet to target ATOMS OS."""
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as s:
            s.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
            try:
                s.bind((SERVER_IP, 0))
            except Exception:
                pass
            for _ in range(3):
                for dest in [target_ip, "192.168.0.222", "192.168.2.255", "192.168.0.255", "255.255.255.255"]:
                    try:
                        s.sendto(b"REBOOT\n", (dest, CONTROL_PORT))
                    except Exception:
                        pass
                time.sleep(0.05)
        log_action("LAN_REBOOT", f"{target_ip}:{CONTROL_PORT}", reason, "SENT_SUCCESS")
        print(f"[CONTROLLER] [REBOOT] Sent REBOOT command to {target_ip}:{CONTROL_PORT}")
        return True
    except Exception as e:
        log_action("LAN_REBOOT", f"{target_ip}:{CONTROL_PORT}", reason, f"ERROR: {e}")
        print(f"[CONTROLLER] [ERROR] Failed to send REBOOT: {e}")
        return False

def send_shutdown_command(target_ip=TARGET_H81_IP, reason="Automated physical test shutdown"):
    """Sends UDP SHUTDOWN command packet to target ATOMS OS."""
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as s:
            s.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
            try:
                s.bind((SERVER_IP, 0))
            except Exception:
                pass
            for _ in range(3):
                s.sendto(b"SHUTDOWN\n", (target_ip, CONTROL_PORT))
                s.sendto(b"SHUTDOWN\n", ("192.168.2.255", CONTROL_PORT))
                time.sleep(0.05)
        log_action("LAN_SHUTDOWN", f"{target_ip}:{CONTROL_PORT}", reason, "SENT_SUCCESS")
        print(f"[CONTROLLER] [SHUTDOWN] Sent SHUTDOWN command to {target_ip}:{CONTROL_PORT}")
        return True
    except Exception as e:
        log_action("LAN_SHUTDOWN", f"{target_ip}:{CONTROL_PORT}", reason, f"ERROR: {e}")
        print(f"[CONTROLLER] [ERROR] Failed to send SHUTDOWN: {e}")
        return False

def send_wake_on_lan(mac=TARGET_MAC):
    """Sends Magic Wake-On-LAN Packet to Target Hardware MAC."""
    try:
        mac_clean = mac.replace(":", "").replace("-", "")
        mac_bytes = bytes.fromhex(mac_clean)
        magic_pkt = b"\xff" * 6 + mac_bytes * 16
        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as s:
            s.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
            try:
                s.bind((SERVER_IP, 0))
            except Exception:
                pass
            for port in [9, 7]:
                for dest in ["192.168.2.255", "255.255.255.255", TARGET_H81_IP]:
                    try:
                        s.sendto(magic_pkt, (dest, port))
                    except Exception:
                        pass
        log_action("WAKE_ON_LAN", f"{mac}", "Wake physical test hardware", "SENT_SUCCESS")
        print(f"[CONTROLLER] [WAKE] Wake-On-LAN magic packet sent to {mac}")
        return True
    except Exception as e:
        log_action("WAKE_ON_LAN", f"{mac}", "Wake physical test hardware", f"ERROR: {e}")
        print(f"[CONTROLLER] [ERROR] Wake-On-LAN failed: {e}")
        return False

def capture_screenshot_one_click(timeout_sec=10.0, output_name=None, session_id=99):
    """
    ONE-CLICK PHYSICAL SCREENSHOT ENGINE
    Dispatches trigger to physical ATOMS TEST PC, receives fragmented UDP 9998 chunks,
    reassembles full framebuffer into BMP, converts to PNG, and prints path.
    Zero manual photo required.
    """
    ensure_output_dir()
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

    try:
        sock.bind((SERVER_IP, SCREENSHOT_PORT))
    except Exception:
        try:
            sock.bind(("0.0.0.0", SCREENSHOT_PORT))
        except Exception as e:
            print(f"[SCREENSHOT ERROR] Could not bind UDP {SCREENSHOT_PORT}: {e}")
            return None

    sock.settimeout(2.5)

    print("\n" + "=" * 70)
    print("  ATOMS OS — ONE-CLICK PHYSICAL SCREENSHOT CAPTURE (UDP 9998)")
    print(f"  Target: {TARGET_H81_IP} | Port: {SCREENSHOT_PORT}")
    print("=" * 70)

    # Dispatch trigger packet
    ctrl = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    ctrl.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
    msg = b"SCREENSHOT\n"
    for dest in [TARGET_H81_IP, "192.168.2.255"]:
        try:
            ctrl.sendto(msg, (dest, CONTROL_PORT))
        except Exception:
            pass
    ctrl.close()
    log_action("TRIGGER_SCREENSHOT", f"{TARGET_H81_IP}:{CONTROL_PORT}", "One-click screenshot capture requested", "TRIGGER_DISPATCHED")
    print(f"[CONTROLLER] Sent 'SCREENSHOT' command to {TARGET_H81_IP}:{CONTROL_PORT}")

    sessions = {}
    start_time = time.time()
    captured_png = None

    while time.time() - start_time < timeout_sec:
        try:
            data, addr = sock.recvfrom(2048)
        except socket.timeout:
            if sessions:
                for s_id, s_data in list(sessions.items()):
                    if s_data['received'] >= (s_data['total'] - 5) and s_data['received'] > 0:
                        break
            continue
        except Exception as e:
            print(f"[ERROR] Socket read error: {e}")
            break

        if len(data) < HEADER_SIZE:
            continue

        header_data = data[:HEADER_SIZE]
        magic, s_id, total_chunks, chunk_index, offset, data_len, flags = struct.unpack(HEADER_FORMAT, header_data)

        if magic != MAGIC_SPMS:
            continue

        chunk_payload = data[HEADER_SIZE:HEADER_SIZE + data_len]

        if s_id not in sessions:
            sessions[s_id] = {
                'chunks': {},
                'total': total_chunks,
                'received': 0,
                'start_time': time.time(),
                'source_ip': addr[0]
            }
            print(f"[RECEIVER] Receiving stream Session {s_id} ({total_chunks} chunks) from {addr[0]}...")

        sess = sessions[s_id]
        if chunk_index not in sess['chunks']:
            sess['chunks'][chunk_index] = (offset, chunk_payload)
            sess['received'] += 1

            if sess['received'] % 250 == 0 or sess['received'] == sess['total']:
                pct = (sess['received'] * 100) // sess['total']
                print(f"[RECEIVER] Progress: {sess['received']}/{sess['total']} chunks ({pct}%)", end='\r', flush=True)

        if sess['received'] >= sess['total'] or ((flags & 2) and sess['received'] >= (sess['total'] - 5)):
            duration = time.time() - sess['start_time']
            print(f"\n[RECEIVER] Received {sess['received']}/{sess['total']} chunks in {duration:.3f}s! Reassembling BMP...", flush=True)

            sorted_chunks = sorted(sess['chunks'].items(), key=lambda x: x[1][0])
            bmp_bytes = bytearray()
            for c_idx, (c_offset, c_data) in sorted_chunks:
                if len(bmp_bytes) < c_offset:
                    bmp_bytes.extend(b'\x00' * (c_offset - len(bmp_bytes)))
                bmp_bytes.extend(c_data)

            if len(bmp_bytes) >= 54 and bmp_bytes[:2] == b'BM':
                file_size, = struct.unpack("<I", bmp_bytes[2:6])
                width, height, planes, bpp = struct.unpack("<iiHH", bmp_bytes[18:30])
                ts_str = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
                base_name = output_name if output_name else f"physical_testpc_{ts_str}"
                bmp_path = os.path.join(OUTPUT_DIR, f"{base_name}.bmp")
                png_path = os.path.join(OUTPUT_DIR, f"{base_name}.png")

                with open(bmp_path, "wb") as f:
                    f.write(bmp_bytes)

                try:
                    from PIL import Image
                    img = Image.open(bmp_path)
                    img.save(png_path)
                    captured_png = png_path
                except ImportError:
                    captured_png = bmp_path

                print("\n" + "=" * 70)
                print("  [SUCCESS] PHYSICAL FRAMEBUFFER CAPTURED")
                print("=" * 70)
                print(f"  Saved Image: {captured_png}")
                print(f"  Resolution : {width}x{abs(height)} ({bpp}-bit)")
                print(f"  File Size  : {len(bmp_bytes):,} bytes")
                print(f"  Duration   : {duration:.3f}s")
                print("=" * 70 + "\n")

                log_action("SCREENSHOT_SUCCESS", os.path.basename(captured_png), f"{width}x{abs(height)} {bpp}bpp", f"SAVED ({len(bmp_bytes)}B)")
                break

    sock.close()
    if not captured_png:
        print("\n[INFO] No screenshot response received within timeout. Target may be unpowered or in non-network state.")
        log_action("SCREENSHOT_TIMEOUT", f"{TARGET_H81_IP}:{SCREENSHOT_PORT}", "Timeout waiting for chunks", "TIMEOUT")
    return captured_png

def run_screenshot_listener(timeout_sec=None, once=False):
    ensure_output_dir()
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

    sock.bind(("0.0.0.0", SCREENSHOT_PORT))

    if timeout_sec:
        sock.settimeout(timeout_sec)

    print("=" * 70)
    print("  ATOMS OS -- FORENSIC SCREENSHOT RECEIVER & TELEMETRY HUB (UDP 9998)")
    print(f"  Listening on: {sock.getsockname()[0]}:{SCREENSHOT_PORT}")
    print(f"  Screenshots Directory: {OUTPUT_DIR}")
    print(f"  Action Ledger:        {LOG_PATH}")
    print("=" * 70)
    print("[RECEIVER] Waiting for ATOMS OS forensic screenshot packets...", flush=True)

    log_action("LISTEN_START", f"UDP:{SCREENSHOT_PORT}", "Awaiting live forensic screenshot from bare-metal", "LISTENING")
    sessions = {}

    try:
        while True:
            try:
                data, addr = sock.recvfrom(2048)
            except socket.timeout:
                print("[RECEIVER] Timeout waiting for screenshot.")
                log_action("SCREENSHOT_RECV", "UDP_STREAM", "Listener timed out", "TIMEOUT")
                return None

            if len(data) < HEADER_SIZE:
                continue

            header_data = data[:HEADER_SIZE]
            magic, session_id, total_chunks, chunk_index, offset, data_len, flags = struct.unpack(HEADER_FORMAT, header_data)

            if magic != MAGIC_SPMS:
                continue

            chunk_payload = data[HEADER_SIZE:HEADER_SIZE + data_len]

            if session_id not in sessions:
                sessions[session_id] = {
                    'chunks': {},
                    'total': total_chunks,
                    'received': 0,
                    'start_time': time.time(),
                    'source_ip': addr[0]
                }
                print(f"\n[RECEIVER] Incoming screenshot stream! Session: {session_id} | Chunks: {total_chunks} from {addr[0]}", flush=True)

            sess = sessions[session_id]
            if chunk_index not in sess['chunks']:
                sess['chunks'][chunk_index] = (offset, chunk_payload)
                sess['received'] += 1

                if sess['received'] % 200 == 0 or sess['received'] == sess['total']:
                    pct = (sess['received'] * 100) // sess['total']
                    print(f"[RECEIVER] Progress: {sess['received']}/{sess['total']} chunks ({pct}%)", end='\r', flush=True)

            if sess['received'] >= sess['total'] or ((flags & 2) and sess['received'] >= (sess['total'] - 5)):
                duration = time.time() - sess['start_time']
                print(f"\n[RECEIVER] All {sess['received']}/{sess['total']} chunks received in {duration:.3f}s! Reassembling BMP...", flush=True)

                sorted_chunks = sorted(sess['chunks'].items(), key=lambda x: x[1][0])
                bmp_bytes = bytearray()
                for c_idx, (c_offset, c_data) in sorted_chunks:
                    if len(bmp_bytes) < c_offset:
                        bmp_bytes.extend(b'\x00' * (c_offset - len(bmp_bytes)))
                    bmp_bytes.extend(c_data)

                if len(bmp_bytes) >= 54 and bmp_bytes[:2] == b'BM':
                    file_size, = struct.unpack("<I", bmp_bytes[2:6])
                    width, height, planes, bpp = struct.unpack("<iiHH", bmp_bytes[18:30])
                    timestamp_str = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
                    filename = f"forensic_screen_{timestamp_str}_s{session_id}.bmp"
                    filepath = os.path.join(OUTPUT_DIR, filename)

                    with open(filepath, "wb") as f:
                        f.write(bmp_bytes)

                    png_path = None
                    try:
                        from PIL import Image
                        img = Image.open(filepath)
                        png_path = filepath.rsplit('.', 1)[0] + ".png"
                        img.save(png_path)
                    except ImportError:
                        pass

                    print("\n" + "=" * 70)
                    print("  FORENSIC SCREENSHOT RECEIVED & VALIDATED")
                    print("=" * 70)
                    print(f"  File       : {filepath}")
                    if png_path:
                        print(f"  PNG Preview: {png_path}")
                    print(f"  Resolution : {width}x{abs(height)} ({bpp}-bit)")
                    print(f"  File Size  : {len(bmp_bytes):,} bytes")
                    print(f"  Duration   : {duration:.3f} seconds")
                    print("=" * 70 + "\n", flush=True)

                    log_action("SCREENSHOT_CAPTURED", filename, f"Full framebuffer {width}x{abs(height)} reassembled", f"SAVED ({len(bmp_bytes)} bytes)")

                    del sessions[session_id]

                    if once:
                        return png_path if png_path else filepath
                else:
                    print(f"[RECEIVER] Warning: Corrupted BMP magic (got {bmp_bytes[:2]}), size={len(bmp_bytes)}")
                    log_action("SCREENSHOT_CORRUPT", f"Session {session_id}", "Invalid BMP header signature", "DISCARDED")
                    del sessions[session_id]

    except KeyboardInterrupt:
        print("\n[RECEIVER] Stopped by user.")
        log_action("LISTEN_STOP", f"UDP:{SCREENSHOT_PORT}", "Stopped by operator", "STOPPED")
    finally:
        sock.close()

def run_autonomous_physical_loop(test_id="phase16b_supervisor_run", objective="Autonomous Physical Loop", timeout_sec=90, case_id="PHYSICAL_CRASH_SUPERVISOR"):
    """
    PHYSICAL TEST LOOP SUPERVISOR:
    DEV LAPTOP -> Physical TEST PC -> PXE Boot -> Boot Detection -> Telemetry Ingest ->
    Crash Detection -> Immediate Framebuffer Capture -> Evidence Bundle Packaging ->
    Automatic Hardware Recovery (Reboot) -> Ready for next run.
    """
    print("\n" + "=" * 78)
    print("  ATOMS OS — AUTONOMOUS PHYSICAL TEST SUPERVISOR")
    print(f"  Target  : Dedicated Physical TEST PC ({TARGET_H81_IP} / {TARGET_MAC})")
    print(f"  Test ID : {test_id}")
    print(f"  Case ID : {case_id}")
    print("=" * 78 + "\n")

    log_action("SUPERVISOR_START", test_id, objective, "INITIALIZING")

    # Step 1: Pre-flight check on host Ethernet IP
    print("[SUPERVISOR] [STEP 1/6] Validating host network interface (192.168.2.1)...")

    # Step 2: Trigger boot or reboot
    print("[SUPERVISOR] [STEP 2/6] Triggering hardware reset / Wake-On-LAN...")
    send_reboot_command(reason="Pre-test reset to enter fresh PXE cycle")
    time.sleep(0.5)
    send_wake_on_lan()

    # Step 3: Listen on UDP 9999 for boot detection and live telemetry
    print(f"[SUPERVISOR] [STEP 3/6] Listening for ATOMS OS boot banner & telemetry (Timeout: {timeout_sec}s)...")
    sock_tel = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock_tel.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    try:
        sock_tel.bind(("0.0.0.0", CONTROL_PORT))
    except Exception as e:
        print(f"[SUPERVISOR] Warning: Could not bind port {CONTROL_PORT}: {e}")

    sock_tel.settimeout(2.0)

    start_time = time.time()
    boot_detected = False
    crash_detected = False
    crash_reason = None
    telemetry_lines = []
    last_packet_time = time.time()

    while time.time() - start_time < timeout_sec:
        try:
            data, addr = sock_tel.recvfrom(2048)
            last_packet_time = time.time()
            line = data.decode("utf-8", errors="replace").strip()
            if line:
                telemetry_lines.append(f"[{datetime.datetime.now().strftime('%H:%M:%S.%f')[:-3]}] {line}")
                print(f"[TEL] {line}")

                if not boot_detected:
                    if any(k in line for k in ["[LANDBG]", "HEARTBEAT", "ROOK", "ATOMS", "BOOT"]):
                        boot_detected = True
                        print(f"\n[SUPERVISOR] PHYSICAL BOOT CONFIRMED from {addr[0]}!\n")

                # Detect crash
                if any(k in line for k in ["=== FAULT DETECTED", "=== EXCEPTION DETECTED", "[CRASH]", "#PF", "#GP", "#UD", "#DF", "PANIC", "ASSERT FAILED"]):
                    crash_detected = True
                    crash_reason = line
                    print(f"\n[SUPERVISOR] [CRASH] DETECTED ON PHYSICAL TEST PC: {crash_reason}\n")
                    break

                if "DESKTOP_VISIBLE" in line or "ROOK_PAGE_DESKTOP" in line or "Starting ATOMS OS Enterprise Desktop" in line:
                    print(f"\n[SUPERVISOR] [DESKTOP] REACHED ON PHYSICAL TEST PC!\n")
                    break
        except socket.timeout:
            if boot_detected and (time.time() - last_packet_time > 15.0):
                crash_detected = True
                crash_reason = "WATCHDOG_HANG (No telemetry received for 15 seconds after boot)"
                print(f"\n[SUPERVISOR] [CRASH] {crash_reason}\n")
                break
            continue

    sock_tel.close()

    # Step 4: Immediately capture framebuffer
    print("[SUPERVISOR] [STEP 4/6] Capturing physical framebuffer...")
    time.sleep(0.5)
    captured_screenshot = capture_screenshot_one_click(timeout_sec=8.0, output_name=f"supervisor_{test_id}")

    # Step 5: Package complete forensic crash bundle
    print("[SUPERVISOR] [STEP 5/6] Assembling forensic evidence bundle...")
    try:
        from forensic_evidence_collector import UnifiedEvidenceCollector
        collector = UnifiedEvidenceCollector(case_id)
        bundle_dir = collector.create_test_bundle(
            test_id=test_id,
            objective=objective,
            bmp_file_path=captured_screenshot if (captured_screenshot and captured_screenshot.endswith('.bmp')) else None,
            telemetry_lines=telemetry_lines
        )
        print(f"[SUPERVISOR] Evidence Bundle created at: {bundle_dir}")
    except Exception as e:
        print(f"[SUPERVISOR] Bundle creation error: {e}")
        bundle_dir = OUTPUT_DIR

    # Step 6: Automatic physical recovery (reboot)
    print("[SUPERVISOR] [STEP 6/6] Executing automatic physical hardware recovery (reboot)...")
    send_reboot_command(reason="Post-test automated reset")

    verdict = "FAIL" if crash_detected else ("PASS" if boot_detected else "TIMEOUT")
    print("\n" + "=" * 78)
    print(f"  AUTONOMOUS PHYSICAL TEST RESULT: {verdict}")
    print(f"  Boot Confirmed : {boot_detected}")
    print(f"  Crash Detected : {crash_detected} ({crash_reason})")
    print(f"  Screenshot     : {captured_screenshot}")
    print(f"  Bundle Dir     : {bundle_dir}")
    print("=" * 78 + "\n")

    log_action("SUPERVISOR_COMPLETE", test_id, objective, f"VERDICT={verdict}")
    return verdict, bundle_dir

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="ATOMS OS Automated Forensic Test Controller & Supervisor")
    parser.add_argument("--listen", action="store_true", help="Run continuous screenshot receiver on UDP 9998")
    parser.add_argument("--capture-screen", "--screenshot", action="store_true", help="ONE-CLICK SCREENSHOT: Trigger & capture physical framebuffer")
    parser.add_argument("--reboot", action="store_true", help="Send remote REBOOT command to physical TEST PC")
    parser.add_argument("--shutdown", action="store_true", help="Send remote SHUTDOWN command to physical TEST PC")
    parser.add_argument("--wake", action="store_true", help="Send Wake-On-LAN magic packet to physical TEST PC")
    parser.add_argument("--supervisor-loop", action="store_true", help="Run autonomous physical test supervisor loop")
    parser.add_argument("--test-id", type=str, default="phys_test_001", help="Test ID for supervisor loop")
    parser.add_argument("--case-id", type=str, default="PHYSICAL_CRASH_SUPERVISOR", help="Case ID for evidence bundle")
    parser.add_argument("--objective", type=str, default="Physical Hardware Automated Telemetry & Capture", help="Test objective")
    args = parser.parse_args()

    if args.capture_screen:
        capture_screenshot_one_click()
    elif args.reboot:
        send_reboot_command()
    elif args.shutdown:
        send_shutdown_command()
    elif args.wake:
        send_wake_on_lan()
    elif args.supervisor_loop:
        run_autonomous_physical_loop(test_id=args.test_id, objective=args.objective, case_id=args.case_id)
    elif args.listen:
        run_screenshot_listener()
    else:
        # Default behavior if no flags: show help or one-click screenshot
        capture_screenshot_one_click()
