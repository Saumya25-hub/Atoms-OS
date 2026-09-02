#!/usr/bin/env python3
"""
ATOMS OS — Autonomous Forensic Runner
Orchestrates hardware wake, boot detection, automated evidence collection,
controlled fix deployment, and lifecycle validation.
"""

import sys
import os
import socket
import struct
import time
import datetime
import argparse
from forensic_evidence_collector import UnifiedEvidenceCollector, SCREENSHOTS_RAW_DIR

try:
    from aipdebug.receiver import AIPDReceiver
    from aipdebug.correlator import AIPDCorrelator
    from aipdebug.packager import AIPDPackager
except ImportError:
    from tools.aipdebug.receiver import AIPDReceiver
    from tools.aipdebug.correlator import AIPDCorrelator
    from tools.aipdebug.packager import AIPDPackager

TARGET_MAC = "A0:AD:9F:C5:81:27"
TARGET_IP = "192.168.2.100"
SERVER_IP = "192.168.2.1"
UDP_PORT = 9999
BUILD_DIR = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "build")
LIVE_LOG = os.path.join(BUILD_DIR, "atoms_live_kernel.log")

def wake_machine():
    print(f"\n[RUNNER] [WAKE] Sending Wake-On-LAN magic packet to {TARGET_MAC}...")
    try:
        mac_clean = TARGET_MAC.replace(":", "").replace("-", "")
        mac_bytes = bytes.fromhex(mac_clean)
        magic_pkt = b"\xff" * 6 + mac_bytes * 16
        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as s:
            s.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
            s.sendto(magic_pkt, ("192.168.2.255", 9))
            s.sendto(magic_pkt, ("255.255.255.255", 9))
            s.sendto(magic_pkt, ("192.168.2.100", 9))
        print(f"[RUNNER] [WAKE] Magic packet broadcast sent successfully.")
        return True
    except Exception as e:
        print(f"[RUNNER] [ERROR] Wake-On-LAN failed: {e}")
        return False

def send_remote_command(cmd):
    print(f"[RUNNER] [CMD] Sending remote command '{cmd}' to {TARGET_IP}:{UDP_PORT}...")
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as s:
            s.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
            msg = (cmd.strip() + "\n").encode('utf-8')
            for _ in range(3):
                s.sendto(msg, (TARGET_IP, UDP_PORT))
                s.sendto(msg, ("192.168.2.255", UDP_PORT))
                time.sleep(0.05)
        print(f"[RUNNER] [CMD] Command '{cmd}' sent.")
        return True
    except Exception as e:
        print(f"[RUNNER] [ERROR] Remote command failed: {e}")
        return False

def wait_for_boot(timeout=60):
    print(f"[RUNNER] [WAIT] Waiting for ATOMS OS to boot on target machine (Timeout: {timeout}s)...")
    start_time = time.time()
    initial_screenshots = set(os.listdir(SCREENSHOTS_RAW_DIR)) if os.path.exists(SCREENSHOTS_RAW_DIR) else set()

    last_log_size = os.path.getsize(LIVE_LOG) if os.path.exists(LIVE_LOG) else 0

    while time.time() - start_time < timeout:
        # Check if new screenshot arrived
        current_screenshots = set(os.listdir(SCREENSHOTS_RAW_DIR)) if os.path.exists(SCREENSHOTS_RAW_DIR) else set()
        new_screenshots = current_screenshots - initial_screenshots
        new_bmps = [s for s in new_screenshots if s.endswith(".bmp")]

        if new_bmps:
            new_bmps.sort(reverse=True)
            print(f"\n[RUNNER] [OK] ATOMS ALIVE & SCREENSHOT DETECTED: {new_bmps[0]}!")
            return os.path.join(SCREENSHOTS_RAW_DIR, new_bmps[0])

        # Check if telemetry is actively arriving
        if os.path.exists(LIVE_LOG):
            current_log_size = os.path.getsize(LIVE_LOG)
            if current_log_size > last_log_size:
                print(f"[RUNNER] [TEL] Active telemetry detected ({current_log_size} bytes)...", end='\r', flush=True)
                last_log_size = current_log_size

        time.sleep(1)

    print(f"\n[RUNNER] [WARN] Timeout waiting for boot/screenshot.")
    return None

def read_recent_telemetry_lines(count=100):
    if not os.path.exists(LIVE_LOG):
        return []
    with open(LIVE_LOG, "r", encoding="utf-8", errors="ignore") as f:
        lines = f.readlines()
    return lines[-count:] if len(lines) > count else lines

def run_forensic_cycle(test_id, objective, do_wake=True, timeout=60):
    print("=" * 65)
    print(f"  ATOMS OS — AUTONOMOUS TEST RUNNER: {test_id}")
    print(f"  Objective: {objective}")
    print("=" * 65)

    aipd_rx = AIPDReceiver()
    aipd_rx.start()

    if do_wake:
        wake_machine()
        print("[RUNNER] Waiting for hardware to power up...")
        time.sleep(3)

    bmp_path = wait_for_boot(timeout=timeout)
    if not bmp_path:
        aipd_rx.stop()
        print(f"[RUNNER] [FAIL] Could not capture screenshot from ATOMS OS.")
        return None

    time.sleep(1.0)
    aipd_rx.stop()

    telemetry = read_recent_telemetry_lines(100)
    collector = UnifiedEvidenceCollector("CASE_20260903_PS2_LED")
    bundle_dir = collector.create_test_bundle(
        test_id=test_id,
        objective=objective,
        bmp_file_path=bmp_path,
        telemetry_lines=telemetry
    )

    # Save and package AIPDebug telemetry
    aipd_rx.save_dump(bundle_dir)
    events = aipd_rx.get_events_dict()
    transactions = AIPDCorrelator.correlate_transactions(events)
    pkg = AIPDPackager.build_package(
        case_id="CASE_20260903_PS2_LED",
        test_id=test_id,
        transactions=transactions,
        events=events,
        screenshot_file="screenshot.png"
    )
    AIPDPackager.save_package(pkg, bundle_dir)

    print(f"[RUNNER] [OK] Test run {test_id} complete! Bundle at: {bundle_dir}")
    return bundle_dir

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Autonomous Forensic Runner")
    parser.add_argument("--wake", action="store_true", help="Send Wake-On-LAN")
    parser.add_argument("--shutdown", action="store_true", help="Send remote SHUTDOWN")
    parser.add_argument("--reboot", action="store_true", help="Send remote REBOOT")
    parser.add_argument("--test-id", type=str, default="test_002_boot_forensic", help="Test ID")
    parser.add_argument("--objective", type=str, default="Live boot forensic capture on Haswell H81 hardware", help="Objective")
    args = parser.parse_args()

    if args.shutdown:
        send_remote_command("SHUTDOWN")
    elif args.reboot:
        send_remote_command("REBOOT")
    elif args.wake:
        wake_machine()
    else:
        run_forensic_cycle(args.test_id, args.objective, do_wake=True, timeout=60)
