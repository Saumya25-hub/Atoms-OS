#!/usr/bin/env python3
"""
ATOMS OS — Forensic Evidence Collector (Milestone 2)
Unified multi-stream evidence ingestion, structured bundling, and case tracking.
"""

import sys
import os
import socket
import struct
import time
import json
import datetime
import re
import subprocess
import argparse

CASES_DIR = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "artifacts", "cases")
SCREENSHOTS_RAW_DIR = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "artifacts", "screenshots")

MAGIC_SPMS = 0x534D5053
HEADER_FORMAT = "<IIHHIHH" # 20 bytes: magic, session_id, total_chunks, chunk_index, offset, data_len, flags
HEADER_SIZE = struct.calcsize(HEADER_FORMAT)

def get_git_commit():
    try:
        res = subprocess.run(["git", "rev-parse", "--short", "HEAD"], capture_output=True, text=True, timeout=2)
        if res.returncode == 0:
            return res.stdout.strip()
    except Exception:
        pass
    return "UNKNOWN"

TESTBENCH_CONFIG_PATH = os.path.join(os.path.dirname(os.path.abspath(__file__)), "testbench_config.json")

def load_authoritative_hardware():
    if os.path.exists(TESTBENCH_CONFIG_PATH):
        try:
            with open(TESTBENCH_CONFIG_PATH, "r", encoding="utf-8") as f:
                data = json.load(f)
                if "target_hardware" in data:
                    return data["target_hardware"]
        except Exception:
            pass
    return {
        "motherboard": "ASUS B750M-K",
        "chipset": "Intel B760/B750 Chipset (LGA1700)",
        "cpu": "Intel Core i3-14100F (Raptor Lake Refresh, 4C/8T)",
        "nic": "Realtek PCIe Gigabit NIC (RTL8168/8111)",
        "mac": "A0:AD:9F:C5:81:27",
        "ip": "192.168.2.100"
    }

class CaseManager:
    def __init__(self, case_id="CASE_20260903_PS2_LED"):
        self.case_id = case_id
        self.case_dir = os.path.join(CASES_DIR, case_id)
        os.makedirs(self.case_dir, exist_ok=True)
        self.manifest_path = os.path.join(self.case_dir, "case_manifest.json")
        self.manifest = self._load_or_init_manifest()

    def _load_or_init_manifest(self):
        auth_hw = load_authoritative_hardware()
        if os.path.exists(self.manifest_path):
            try:
                with open(self.manifest_path, "r", encoding="utf-8") as f:
                    data = json.load(f)
                    if "target_hardware" in data:
                        curr_mb = data["target_hardware"].get("motherboard", "")
                        if "H81" in curr_mb or "LGA1150" in curr_mb:
                            if "metadata_corrections" not in data:
                                data["metadata_corrections"] = []
                            data["metadata_corrections"].append({
                                "applied_at": datetime.datetime.now().isoformat(),
                                "prior_hardware": data["target_hardware"],
                                "authoritative_hardware": auth_hw,
                                "reason": "Hardware identity corrected from erroneous H81/Haswell inference to verified physical ASUS B750M-K with Intel Core i3-14100F (LGA1700). Historical audit preserved."
                            })
                            data["target_hardware"] = auth_hw
                    return data
            except Exception:
                pass
        return {
            "case_id": self.case_id,
            "title": "ATOMS OS PS/2 Keyboard Initialization & LED Forensics",
            "target_hardware": auth_hw,
            "created_at": datetime.datetime.now().isoformat(),
            "last_updated": datetime.datetime.now().isoformat(),
            "status": "INVESTIGATING",
            "confirmed_findings": [
                "8042 controller microcontroller self-test (0xAA) passes returning 0x55 in ~60ms.",
                "Physical serial Clock/Data interface lines (0xAB) pass returning 0x00 in ~320us.",
                "Pre-reset port 0x60 buffer is clean (0 stale bytes).",
                "Keyboard device reset (0xFF) times out on the wire after exactly ~240ms, returning controller-generated 0xFE (RESEND).",
                "0xAA self-test clears timeout bit 6 from 0x7C down to 0x1C.",
                "0xAA self-test resets controller command byte to 0x30 (inhibiting clocks).",
                "Gated state machine correctly skips 0xF4, 0xEE, and 0xED when BAT does not complete, preventing cascading bus corruption."
            ],
            "disproven_hypotheses": [
                "Mouse byte swallowing keyboard ACK (Disproven: Mouse bytes are now verified AUX=1 and isolated).",
                "Stale 0xFE leftover in port 0x60 buffer (Disproven: Pre-reset drain showed exactly 0 bytes).",
                "LED subsystem bug (Disproven: Failure occurs on 0xFF reset, 0xF4 scan, and 0xEE echo before LED command)."
            ],
            "active_hypotheses": [
                "Keyboard clock line is inhibited by post-0xAA configuration 0x30 before device reset transaction.",
                "Port 0xAE enable command timing requires hardware recovery delay before 0xFF transmission.",
                "Keyboard device requires power-on settling delay before accepting 0xFF reset."
            ],
            "test_history": []
        }

    def save_manifest(self):
        self.manifest["last_updated"] = datetime.datetime.now().isoformat()
        with open(self.manifest_path, "w", encoding="utf-8") as f:
            json.dump(self.manifest, f, indent=2)

    def record_test(self, test_summary):
        self.manifest["test_history"].append(test_summary)
        self.save_manifest()


class EvidenceParser:
    @staticmethod
    def parse_telemetry(log_lines):
        summary = {
            "controller_self_test": None,
            "interface_test": None,
            "reset_state_machine": {
                "pre_reset_drained": 0,
                "retries": [],
                "final_state": "UNKNOWN"
            },
            "status_transitions": {},
            "controller_config": {},
            "gated_commands": {},
            "verdict": "UNKNOWN"
        }

        full_text = "\n".join(log_lines)

        # Parse 0xAA
        m = re.search(r"Self-Test\s*\(CMD 0xAA\)\s*:\s*(0x[0-9A-Fa-f]+)\s*\[([^\]]+)\]\s*(\d+)\s*us", full_text)
        if m:
            summary["controller_self_test"] = {"cmd": "0xAA", "response": m.group(1), "status": m.group(2).strip(), "latency_us": int(m.group(3))}

        # Parse 0xAB
        m = re.search(r"Interface\s*\(CMD 0xAB\)\s*:\s*(0x[0-9A-Fa-f]+)\s*\[([^\]]+)\]\s*(\d+)\s*us", full_text)
        if m:
            summary["interface_test"] = {"cmd": "0xAB", "response": m.group(1), "status": m.group(2).strip(), "latency_us": int(m.group(3))}

        # Parse Pre-Reset Drained
        m = re.search(r"Pre-Reset Drained\s*:\s*(\d+)\s*bytes", full_text)
        if m:
            summary["reset_state_machine"]["pre_reset_drained"] = int(m.group(1))

        # Parse Retries
        for m in re.finditer(r"TX 0xFF\s*\[(R\d+)\]\s*:\s*(0x[0-9A-Fa-f]+)\s*\[([^\]]+)\]\s*(\d+)\s*us", full_text):
            summary["reset_state_machine"]["retries"].append({
                "retry": m.group(1),
                "response": m.group(2),
                "meaning": m.group(3).strip(),
                "latency_us": int(m.group(4))
            })

        # Parse Current Reset State
        m = re.search(r"Current Reset State\s*:\s*([A-Za-z0-9_]+)", full_text)
        if m:
            summary["reset_state_machine"]["final_state"] = m.group(1).strip()

        # Parse Status Transitions
        m = re.search(r"T0 Boot Status\s*:\s*(0x[0-9A-Fa-f]+)", full_text)
        if m: summary["status_transitions"]["t0"] = m.group(1)
        m = re.search(r"After 0xAA Test\s*:\s*(0x[0-9A-Fa-f]+)", full_text)
        if m: summary["status_transitions"]["after_0xaa"] = m.group(1)
        m = re.search(r"Final Status Byte\s*:\s*(0x[0-9A-Fa-f]+)", full_text)
        if m: summary["status_transitions"]["final"] = m.group(1)

        # Parse Config Transitions
        m = re.search(r"Initial Config \(T0\)\s*:\s*(0x[0-9A-Fa-f]+)", full_text)
        if m: summary["controller_config"]["t0"] = m.group(1)
        m = re.search(r"Post-0xAA Config\s*:\s*(0x[0-9A-Fa-f]+)", full_text)
        if m: summary["controller_config"]["post_0xaa"] = m.group(1)

        # Parse Gated Commands
        m = re.search(r"Enable Scan \(0xF4\)\s*:\s*([^\r\n]+)", full_text)
        if m: summary["gated_commands"]["f4_scan"] = m.group(1).strip()
        m = re.search(r"0xEE Echo Probe\s*:\s*([^\r\n]+)", full_text)
        if m: summary["gated_commands"]["ee_echo"] = m.group(1).strip()
        m = re.search(r"LED Command Status\s*:\s*([^\r\n]+)", full_text)
        if m: summary["gated_commands"]["ed_led"] = m.group(1).strip()

        # Parse Verdict
        m = re.search(r"FINAL VERDICT\s*:\s*([^\r\n]+)", full_text)
        if m:
            summary["verdict"] = m.group(1).strip()

        return summary


class UnifiedEvidenceCollector:
    def __init__(self, case_id="CASE_20260903_PS2_LED"):
        self.case_mgr = CaseManager(case_id)
        self.active_test_id = None
        self.active_test_dir = None
        self.telemetry_buffer = []

    def create_test_bundle(self, test_id, objective, bmp_bytes=None, bmp_file_path=None, telemetry_lines=None, serial_log_path=None):
        self.active_test_id = test_id
        self.active_test_dir = os.path.join(self.case_mgr.case_dir, test_id)
        os.makedirs(self.active_test_dir, exist_ok=True)

        now_str = datetime.datetime.now().isoformat()
        git_hash = get_git_commit()

        print(f"\n[COLLECTOR] ========================================================")
        print(f"[COLLECTOR] PACKAGING EVIDENCE BUNDLE: {test_id}")
        print(f"[COLLECTOR] Destination: {self.active_test_dir}")
        print(f"[COLLECTOR] ========================================================")

        # 1. Process Screenshot
        dest_bmp = os.path.join(self.active_test_dir, "screenshot.bmp")
        dest_png = os.path.join(self.active_test_dir, "screenshot.png")
        visual_meta = {"present": False}

        if bmp_bytes:
            with open(dest_bmp, "wb") as f:
                f.write(bmp_bytes)
        elif bmp_file_path and os.path.exists(bmp_file_path):
            with open(bmp_file_path, "rb") as f_in, open(dest_bmp, "wb") as f_out:
                f_out.write(f_in.read())

        if os.path.exists(dest_bmp):
            size_bytes = os.path.getsize(dest_bmp)
            with open(dest_bmp, "rb") as f:
                header = f.read(54)
                if len(header) >= 54 and header[:2] == b'BM':
                    width, height, planes, bpp = struct.unpack("<iiHH", header[18:30])
                    visual_meta = {
                        "present": True,
                        "file_bmp": "screenshot.bmp",
                        "file_png": "screenshot.png",
                        "resolution": f"{width}x{abs(height)}",
                        "bpp": bpp,
                        "size_bytes": size_bytes
                    }
            # Convert to PNG
            try:
                from PIL import Image
                img = Image.open(dest_bmp)
                img.save(dest_png)
                print(f"[COLLECTOR] [OK] Visual Artifact Saved: screenshot.png ({visual_meta['resolution']}, {bpp}-bit)")
            except Exception as e:
                print(f"[COLLECTOR] [WARN] PNG conversion: {e}")

        # 2. Process Telemetry Log
        dest_telemetry = os.path.join(self.active_test_dir, "telemetry.log")
        lines = telemetry_lines if telemetry_lines else []
        with open(dest_telemetry, "w", encoding="utf-8") as f:
            for line in lines:
                f.write(line.strip() + "\n")
        print(f"[COLLECTOR] [OK] Telemetry Log Saved: telemetry.log ({len(lines)} records)")

        # 3. Process Serial Log if available
        dest_serial = os.path.join(self.active_test_dir, "serial.log")
        if serial_log_path and os.path.exists(serial_log_path):
            with open(serial_log_path, "r", encoding="utf-8", errors="ignore") as f_in, open(dest_serial, "w", encoding="utf-8") as f_out:
                f_out.write(f_in.read())
            print(f"[COLLECTOR] [OK] Serial Log Saved: serial.log")

        # 4. Parse Structured Evidence
        evidence_summary = EvidenceParser.parse_telemetry(lines)
        evidence_summary_path = os.path.join(self.active_test_dir, "evidence_summary.json")
        with open(evidence_summary_path, "w", encoding="utf-8") as f:
            json.dump(evidence_summary, f, indent=2)
        print(f"[COLLECTOR] [OK] Machine-Readable Evidence Generated: evidence_summary.json")

        # 5. Generate Metadata
        metadata = {
            "test_id": test_id,
            "case_id": self.case_mgr.case_id,
            "objective": objective,
            "git_commit": git_hash,
            "timestamp": now_str,
            "target": load_authoritative_hardware(),
            "target_source": "tools/testbench_config.json",
            "visual": visual_meta,
            "verdict": evidence_summary.get("verdict", "UNKNOWN")
        }
        metadata_path = os.path.join(self.active_test_dir, "metadata.json")
        with open(metadata_path, "w", encoding="utf-8") as f:
            json.dump(metadata, f, indent=2)
        print(f"[COLLECTOR] [OK] Metadata Manifest Saved: metadata.json")

        # 6. Update Case Manifest
        test_record = {
            "test_id": test_id,
            "objective": objective,
            "timestamp": now_str,
            "git_commit": git_hash,
            "verdict": evidence_summary.get("verdict", "UNKNOWN"),
            "artifacts_folder": os.path.relpath(self.active_test_dir, os.path.dirname(self.case_mgr.case_dir)),
            "summary": evidence_summary
        }
        self.case_mgr.record_test(test_record)

        print(f"[COLLECTOR] ========================================================")
        print(f"[COLLECTOR] EVIDENCE BUNDLE COMPLETE: {test_id}")
        print(f"[COLLECTOR] Final Verdict: {test_record['verdict']}")
        print(f"[COLLECTOR] ========================================================\n")
        return self.active_test_dir

def ingest_existing_real_hardware_run():
    collector = UnifiedEvidenceCollector("CASE_20260903_PS2_LED")
    
    # Locate recent BMP from Milestone 1
    raw_bmps = [f for f in os.listdir(SCREENSHOTS_RAW_DIR) if f.endswith(".bmp")]
    raw_bmps.sort(reverse=True)
    if not raw_bmps:
        print("[INGEST] No existing raw BMP found in artifacts/screenshots.")
        return

    latest_bmp = os.path.join(SCREENSHOTS_RAW_DIR, raw_bmps[0])
    
    # Baseline telemetry extracted from real-hardware run
    telemetry_baseline = [
        "8042 CONTROLLER HARDWARE TESTS",
        "---------------------------------------------",
        "Self-Test (CMD 0xAA) : 0x55  [0x55 PASS] 60195 us",
        "Interface (CMD 0xAB) : 0x00  [LINES OK]  323   us",
        "KEYBOARD RESET & BAT STATE MACHINE (0xFF)",
        "---------------------------------------------",
        "Pre-Reset Drained    : 0 bytes",
        "TX 0xFF [R0]         : 0xFE  [RESEND]     240218 us",
        "TX 0xFF [R1]         : 0xFE  [RESEND]     240166 us",
        "Current Reset State  : RESEND",
        "STATUS REGISTER 0x64 TRANSITIONS",
        "---------------------------------------------",
        "T0 Boot Status       : 0x7C  [BIT 6 TIMEOUT]",
        "After 0xAA Test      : 0x1C  [TIMEOUT CLEARED]",
        "Final Status Byte    : 0x54  [TIMEOUT SET]",
        "CONTROLLER CONFIGURATION",
        "---------------------------------------------",
        "Initial Config (T0)  : 0x47",
        "Post-0xAA Config     : 0x30",
        "SCANNING & COMMAND CHANNEL HEALTH",
        "---------------------------------------------",
        "Enable Scan (0xF4)   : SKIPPED (NO BAT PASS)",
        "0xEE Echo Probe      : BLOCKED (SCAN NOT PASS)",
        "0xED KEYBOARD LED TRANSACTION",
        "---------------------------------------------",
        "LED Command Status   : BLOCKED (CHANNEL UNHEALTHY)",
        "LAN LIVE TELEMETRY (RTL8168 / GbE)",
        "---------------------------------------------",
        "NIC Device / Link    : Realtek GbE [1000 Mbps FD]",
        "Target MAC Address   : A0:AD:9F:C5:81:27",
        "RX / TX Frames       : 0   RX  / 100   TX",
        "CERTIFICATION VERDICT",
        "---------------------------------------------",
        "FINAL VERDICT        : FAIL [AWAITING PROTOCOL PASS]"
    ]

    collector.create_test_bundle(
        test_id="test_001_initial_audit",
        objective="Baseline 8042 controller diagnostic and keyboard reset/BAT state machine audit on Haswell H81 hardware",
        bmp_file_path=latest_bmp,
        telemetry_lines=telemetry_baseline
    )

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="ATOMS OS Unified Forensic Evidence Collector")
    parser.add_argument("--ingest-baseline", action="store_true", help="Ingest Milestone 1 real-hardware run into structured case")
    parser.add_argument("--case", type=str, default="CASE_20260903_PS2_LED", help="Case ID")
    parser.add_argument("--test", type=str, default="test_001_initial_audit", help="Test ID")
    args = parser.parse_args()

    if args.ingest_baseline:
        ingest_existing_real_hardware_run()
    else:
        ingest_existing_real_hardware_run()
