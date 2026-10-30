#!/usr/bin/env python3
"""
ATOMS OS — Live Forensic Telemetry & FreeBSD Dmesg Receiver (Stage D0)
Copyright © 2026 ATOMS OS Project / Saumya Chaudhari

Dual UDP Stream Receiver:
  - UDP Port 9999: ATOMS Hypervisor Diagnostic Telemetry (#<seq> [<tsc>] [<TAG>] <msg>)
  - UDP Port 9995: FreeBSD Live Kernel Console / Dmesg Buffer Stream
"""

import sys
import os
import socket
import threading
import datetime
import time
import re

REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
LOG_DIR = os.path.join(REPO_ROOT, "logs")
os.makedirs(LOG_DIR, exist_ok=True)

TIMESTAMP = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
ATOMS_LOG_PATH = os.path.join(LOG_DIR, f"atoms_{TIMESTAMP}.log")
FREEBSD_LOG_PATH = os.path.join(LOG_DIR, f"freebsd_{TIMESTAMP}.log")

# ANSI Color Codes
C_RESET   = "\033[0m"
C_BOLD    = "\033[1m"
C_RED     = "\033[91m"
C_GREEN   = "\033[92m"
C_YELLOW  = "\033[93m"
C_BLUE    = "\033[94m"
C_MAGENTA = "\033[95m"
C_CYAN    = "\033[96m"
C_WHITE   = "\033[97m"
C_DIM     = "\033[2m"

TAG_COLORS = {
    "HV": C_MAGENTA,
    "HID": C_BLUE,
    "FOCUS": C_CYAN,
    "I8042": C_YELLOW,
    "IRQ": C_RED + C_BOLD,
    "IOAPIC": C_MAGENTA + C_BOLD,
    "PIC": C_YELLOW + C_BOLD,
    "PCI": C_BLUE,
    "ACPI": C_GREEN,
    "BUILD": C_WHITE + C_BOLD,
    "GUEST": C_GREEN,
}

# Forensic Tracker State
stats = {
    "atoms_pkts": 0,
    "freebsd_pkts": 0,
    "freebsd_bytes": 0,
    "seq_drops": 0,
    "last_seq": 0,
    "first_i8042_time": None,
    "last_rte1": "UNKNOWN",
    "last_rte4": "UNKNOWN",
    "isa0_found": False,
    "atkbdc0_found": False,
    "atkbd0_found": False,
    "uart0_found": False,
    "panic_detected": False,
    "panic_line": "",
}

running = True

def print_banner():
    print(f"{C_CYAN}{C_BOLD}========================================================================{C_RESET}")
    print(f"{C_WHITE}{C_BOLD}  ATOMS OS — DUAL UDP FORENSIC RECEIVER (STAGE D0){C_RESET}")
    print(f"{C_CYAN}{C_BOLD}========================================================================{C_RESET}")
    print(f"  {C_WHITE}Listening on UDP Ports:{C_RESET}")
    print(f"    - {C_CYAN}0.0.0.0:9999{C_RESET} -> ATOMS Telemetry  -> {ATOMS_LOG_PATH}")
    print(f"    - {C_GREEN}0.0.0.0:9995{C_RESET} -> FreeBSD Console  -> {FREEBSD_LOG_PATH}")
    print(f"  {C_YELLOW}Press Ctrl+C at any time to finish and print forensic summary.{C_RESET}")
    print(f"{C_CYAN}{C_BOLD}========================================================================{C_RESET}\n", flush=True)

def atoms_telemetry_thread(f_out):
    global running
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    if sys.platform == "win32":
        try:
            sock.ioctl(0x9800000C, False) # SIO_UDP_CONNRESET
        except Exception:
            pass
    try:
        sock.bind(("0.0.0.0", 9999))
    except Exception as e:
        print(f"{C_RED}[ERROR] Could not bind to UDP 9999: {e}{C_RESET}", flush=True)
        return

    sock.settimeout(0.5)

    while running:
        try:
            data, addr = sock.recvfrom(4096)
        except socket.timeout:
            continue
        except Exception:
            break

        stats["atoms_pkts"] += 1
        raw_text = data.decode("utf-8", errors="replace")
        f_out.write(raw_text)
        f_out.flush()

        for line in raw_text.splitlines():
            line_str = line.strip()
            if not line_str:
                continue

            # Parse #<seq> [<tsc>] [<TAG>] <msg>
            m = re.match(r"^#(\d+)\s+\[(\d+)\]\s+\[([A-Za-z0-9_]+)\]\s+(.*)$", line_str)
            if m:
                seq = int(m.group(1))
                tsc = int(m.group(2))
                tag = m.group(3)
                msg = m.group(4)

                # Sequence drop detector
                if stats["last_seq"] > 0 and seq > stats["last_seq"] + 1:
                    dropped = seq - stats["last_seq"] - 1
                    stats["seq_drops"] += dropped
                    print(f"{C_RED}{C_BOLD}*** [TELEMETRY DROP DETECTED: {dropped} packets lost (#{stats['last_seq']+1}..#{seq-1})] ***{C_RESET}")

                stats["last_seq"] = seq

                # Forensic state trackers
                if tag == "I8042" and not stats["first_i8042_time"]:
                    stats["first_i8042_time"] = datetime.datetime.now().strftime("%H:%M:%S.%f")[:-3]
                if tag == "IOAPIC":
                    if "RTE[1]" in msg:
                        stats["last_rte1"] = msg
                    elif "RTE[4]" in msg:
                        stats["last_rte4"] = msg

                color = TAG_COLORS.get(tag, C_WHITE)
                print(f"{C_DIM}#{seq:05d}{C_RESET} {color}[{tag:^6s}]{C_RESET} {msg}", flush=True)
            else:
                # Legacy unformatted line
                print(f"{C_DIM}[ATOMS]{C_RESET} {line_str}", flush=True)

def freebsd_console_thread(f_out):
    global running
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    if sys.platform == "win32":
        try:
            sock.ioctl(0x9800000C, False)
        except Exception:
            pass
    try:
        sock.bind(("0.0.0.0", 9995))
    except Exception as e:
        print(f"{C_RED}[ERROR] Could not bind to UDP 9995: {e}{C_RESET}", flush=True)
        return

    sock.settimeout(0.5)

    while running:
        try:
            data, addr = sock.recvfrom(4096)
        except socket.timeout:
            continue
        except Exception:
            break

        stats["freebsd_pkts"] += 1
        stats["freebsd_bytes"] += len(data)
        raw_text = data.decode("utf-8", errors="replace")
        f_out.write(raw_text)
        f_out.flush()

        for line in raw_text.splitlines():
            line_str = line.strip()
            if not line_str:
                continue

            # Check for critical forensic milestones
            l_lower = line_str.lower()
            if "isa0" in l_lower or "isa <isa" in l_lower:
                stats["isa0_found"] = True
            if "atkbdc0" in l_lower or "atkbdc <" in l_lower:
                stats["atkbdc0_found"] = True
            if "atkbd0" in l_lower or "atkbd <" in l_lower:
                stats["atkbd0_found"] = True
            if "uart0" in l_lower or "uart <" in l_lower:
                stats["uart0_found"] = True
            if "panic:" in l_lower or "fatal trap" in l_lower:
                stats["panic_detected"] = True
                stats["panic_line"] = line_str

            print(f"{C_GREEN}{C_BOLD}[FREEBSD]{C_RESET} {line_str}", flush=True)

def print_summary():
    print(f"\n{C_CYAN}{C_BOLD}========================================================================{C_RESET}")
    print(f"{C_WHITE}{C_BOLD}  STAGE D0 — FORENSIC TELEMETRY SUMMARY{C_RESET}")
    print(f"{C_CYAN}{C_BOLD}========================================================================{C_RESET}")
    print(f"  ATOMS Packets Received   : {C_BOLD}{stats['atoms_pkts']}{C_RESET}")
    print(f"  ATOMS Dropped Packets    : {C_RED if stats['seq_drops'] > 0 else C_GREEN}{stats['seq_drops']}{C_RESET}")
    print(f"  FreeBSD Packets Received : {C_BOLD}{stats['freebsd_pkts']}{C_RESET} ({stats['freebsd_bytes']} bytes)")
    print(f"  First i8042 Access Time  : {C_YELLOW}{stats['first_i8042_time'] or 'NONE'}{C_RESET}")
    print(f"  Last IOAPIC RTE[1] (KBD) : {C_CYAN}{stats['last_rte1']}{C_RESET}")
    print(f"  Last IOAPIC RTE[4] (UART): {C_CYAN}{stats['last_rte4']}{C_RESET}")
    print(f"  ----------------------------------------------------------------------")
    print(f"  DEVICE ATTACH STATUS:")
    print(f"    - isa0    (ISA Bus Bridge) : {C_GREEN + 'FOUND / ATTACHED' if stats['isa0_found'] else C_RED + 'NOT DETECTED'}{C_RESET}")
    print(f"    - atkbdc0 (i8042 Ctrl)     : {C_GREEN + 'FOUND / ATTACHED' if stats['atkbdc0_found'] else C_RED + 'NOT DETECTED'}{C_RESET}")
    print(f"    - atkbd0  (PS/2 Keyboard)  : {C_GREEN + 'FOUND / ATTACHED' if stats['atkbd0_found'] else C_RED + 'NOT DETECTED'}{C_RESET}")
    print(f"    - uart0   (COM1 0x3F8)     : {C_GREEN + 'FOUND / ATTACHED' if stats['uart0_found'] else C_RED + 'NOT DETECTED'}{C_RESET}")
    print(f"  ----------------------------------------------------------------------")
    if stats["panic_detected"]:
        print(f"  {C_RED}{C_BOLD}*** GUEST PANIC DETECTED ***{C_RESET}")
        print(f"  {C_RED}{stats['panic_line']}{C_RESET}")
    else:
        print(f"  Guest Panic Status       : {C_GREEN}NO PANIC DETECTED{C_RESET}")
    print(f"  ----------------------------------------------------------------------")
    print(f"  Saved Logs:")
    print(f"    - ATOMS Telemetry : {ATOMS_LOG_PATH}")
    print(f"    - FreeBSD Console : {FREEBSD_LOG_PATH}")
    print(f"{C_CYAN}{C_BOLD}========================================================================{C_RESET}\n")

def main():
    global running
    print_banner()

    f_atoms = open(ATOMS_LOG_PATH, "w", encoding="utf-8", errors="replace")
    f_freebsd = open(FREEBSD_LOG_PATH, "w", encoding="utf-8", errors="replace")

    t_atoms = threading.Thread(target=atoms_telemetry_thread, args=(f_atoms,), daemon=True)
    t_freebsd = threading.Thread(target=freebsd_console_thread, args=(f_freebsd,), daemon=True)

    t_atoms.start()
    t_freebsd.start()

    try:
        while running:
            time.sleep(0.2)
    except KeyboardInterrupt:
        print(f"\n{C_YELLOW}Shutting down forensic receiver...{C_RESET}")
        running = False

    t_atoms.join(timeout=1.0)
    t_freebsd.join(timeout=1.0)
    f_atoms.close()
    f_freebsd.close()

    print_summary()

if __name__ == "__main__":
    main()

