#!/usr/bin/env python3
"""
ATOMS OS — ATOMS Bridge Client
Live Guest VM State Inspection & Control Channel

Communicates over UDP port 9998 with ATOMS OS hypervisor host context.
Commands execute only in VM-exit host context — VMCS is never touched externally.

Usage:
    python tools/bridge.py state
    python tools/bridge.py mem gpa <addr> [len]
    python tools/bridge.py mem gva <addr> [len]
    python tools/bridge.py idt [vec]
    python tools/bridge.py rips [--raw] [--top N]
    python tools/bridge.py type "<text>"
    python tools/bridge.py reboot
    python tools/bridge.py interactive
"""

import sys
import os
import socket
import argparse
import subprocess
import shutil
import bisect
from collections import Counter

DEFAULT_IP = os.environ.get("ATOMS_IP", "192.168.2.50")
DEFAULT_PORT = 9998
DEFAULT_TIMEOUT = 2.5

REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
DEFAULT_KERNEL_PATHS = [
    os.path.join(REPO_ROOT, "tools", "freebsd_payload", "freebsd14_kernel.elf"),
    os.path.join(REPO_ROOT, "tools", "freebsd_payload", "kernel.elf"),
]


def find_llvm_nm():
    """Locate llvm-nm or nm executable."""
    llvm_path = r"C:\Program Files\LLVM\bin\llvm-nm.exe"
    if os.path.exists(llvm_path):
        return llvm_path
    which_nm = shutil.which("llvm-nm") or shutil.which("nm")
    if which_nm:
        return which_nm
    return None


def load_kernel_symbols(kernel_path=None):
    """Extract and sort function symbols from FreeBSD kernel ELF."""
    if not kernel_path:
        for kp in DEFAULT_KERNEL_PATHS:
            if os.path.exists(kp):
                kernel_path = kp
                break

    if not kernel_path or not os.path.exists(kernel_path):
        return [], []

    nm_bin = find_llvm_nm()
    if not nm_bin:
        print("[WARN] llvm-nm/nm not found; RIP symbol resolution disabled.", file=sys.stderr)
        return [], []

    try:
        res = subprocess.run(
            [nm_bin, "-n", kernel_path],
            capture_output=True,
            text=True,
            check=True,
            timeout=30
        )
    except Exception as e:
        print(f"[WARN] Failed to run {nm_bin} on {kernel_path}: {e}", file=sys.stderr)
        return [], []

    symbols = []
    # Parse lines: "<hex_addr> <type> <name>"
    for line in res.stdout.splitlines():
        parts = line.split()
        if len(parts) >= 3:
            addr_str, sym_type, sym_name = parts[0], parts[1], parts[2]
            if sym_type in ('T', 't', 'W', 'w'):
                try:
                    addr = int(addr_str, 16)
                    symbols.append((addr, sym_name))
                except ValueError:
                    continue

    symbols.sort(key=lambda s: s[0])
    addrs = [s[0] for s in symbols]
    return symbols, addrs


def resolve_rip(rip, symbols, addrs):
    """Map a RIP to the closest function symbol + offset."""
    if not addrs:
        return f"0x{rip:016x}", "unknown"

    idx = bisect.bisect_right(addrs, rip) - 1
    if idx >= 0:
        sym_addr, sym_name = symbols[idx]
        offset = rip - sym_addr
        if offset < 0x20000:
            return f"{sym_name}+0x{offset:x}", sym_name
    return f"0x{rip:016x}", "unknown"


def send_bridge_cmd(cmd_str, target_ip=DEFAULT_IP, target_port=DEFAULT_PORT, timeout=DEFAULT_TIMEOUT, multi_packet=False, max_expected=256):
    """Send UDP command to ATOMS Bridge and return collected response text."""
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.settimeout(timeout)

    payload = (cmd_str.strip() + "\n").encode("utf-8")
    sock.sendto(payload, (target_ip, target_port))

    received_parts = []
    total_received_rips = 0
    expected_rips = None

    try:
        while True:
            try:
                data, addr = sock.recvfrom(2048)
                text = data.decode("utf-8", errors="replace")
                received_parts.append(text)

                if multi_packet:
                    for line in text.splitlines():
                        line = line.strip()
                        if line.startswith("rip_count="):
                            try:
                                expected_rips = int(line.split("=")[1])
                            except ValueError:
                                pass
                        elif line.startswith("0x"):
                            total_received_rips += 1

                    if expected_rips is not None and total_received_rips >= expected_rips:
                        break
                    # Shorter timeout between chunks
                    sock.settimeout(0.6)
                else:
                    break
            except socket.timeout:
                break
    finally:
        sock.close()

    return "".join(received_parts)


def cmd_state(args):
    resp = send_bridge_cmd("state", args.ip, args.port, args.timeout)
    if not resp:
        print("[ERROR] No response from ATOMS Bridge (check IP/network).")
        return 1
    print("=== ATOMS GUEST VM STATE ===")
    print(resp.strip())
    return 0


def cmd_mem(args):
    addr_arg = args.addr
    len_arg = str(args.len) if args.len else "64"
    cmd_str = f"mem {args.kind} {addr_arg} {len_arg}"
    resp = send_bridge_cmd(cmd_str, args.ip, args.port, args.timeout)
    if not resp:
        print("[ERROR] No response from ATOMS Bridge.")
        return 1
    print(f"=== ATOMS MEMORY DUMP ({args.kind.upper()} {addr_arg}) ===")
    print(resp.strip())
    return 0


def cmd_idt(args):
    vec_arg = str(args.vec) if args.vec is not None else "0"
    cmd_str = f"idt {vec_arg}"
    resp = send_bridge_cmd(cmd_str, args.ip, args.port, args.timeout)
    if not resp:
        print("[ERROR] No response from ATOMS Bridge.")
        return 1
    print(f"=== GUEST IDT VECTOR {vec_arg} ===")
    print(resp.strip())
    return 0


def cmd_type(args):
    text_to_type = " ".join(args.text)
    cmd_str = f"type {text_to_type}"
    resp = send_bridge_cmd(cmd_str, args.ip, args.port, args.timeout)
    if not resp:
        print("[ERROR] No response from ATOMS Bridge.")
        return 1
    print(resp.strip())
    return 0


def cmd_reboot(args):
    resp = send_bridge_cmd("reboot", args.ip, args.port, args.timeout)
    if not resp:
        print("[ERROR] No response from ATOMS Bridge.")
        return 1
    print(resp.strip())
    return 0


def cmd_rips(args):
    resp = send_bridge_cmd("rips", args.ip, args.port, args.timeout, multi_packet=True)
    if not resp:
        print("[ERROR] No response from ATOMS Bridge.")
        return 1

    lines = resp.splitlines()
    rip_list = []
    total_reported = 0

    for line in lines:
        line = line.strip()
        if line.startswith("rip_count="):
            try:
                total_reported = int(line.split("=")[1])
            except ValueError:
                pass
        elif line.startswith("0x"):
            try:
                rip_list.append(int(line, 16))
            except ValueError:
                pass

    if not rip_list:
        print(f"[BRIDGE] No exit RIPs recorded yet (reported count={total_reported}).")
        return 0

    # Load kernel symbols
    symbols, addrs = load_kernel_symbols(args.kernel)
    if symbols:
        print(f"[BRIDGE] Loaded {len(symbols)} kernel symbols from ELF.")
    else:
        print("[BRIDGE] Warning: Running without symbol resolution.")

    # Resolve all RIPs
    resolved_rips = []
    func_counts = Counter()

    for rip in rip_list:
        full_str, func_name = resolve_rip(rip, symbols, addrs)
        resolved_rips.append((rip, full_str, func_name))
        func_counts[func_name] += 1

    total_rips = len(rip_list)

    print("\n" + "=" * 70)
    print(f" TOP FUNCTIONS HISTOGRAM ({total_rips} Exit RIPs Captured)")
    print("=" * 70)
    print(f" {'COUNT':>6}  {'PERCENT':>7}   {'FUNCTION'}")
    print("-" * 70)

    top_n = args.top if args.top else 15
    for func, cnt in func_counts.most_common(top_n):
        pct = (cnt / total_rips) * 100.0
        bar = "#" * int(pct / 4)
        print(f" {cnt:>6}  {pct:>6.1f}%   {func:<35} {bar}")

    print("=" * 70)

    if args.raw:
        print("\n--- RAW EXIT RIP TRACE (Most recent last) ---")
        for idx, (rip, full_str, _) in enumerate(resolved_rips, 1):
            print(f" [{idx:>3}] 0x{rip:016x} -> {full_str}")

    return 0


def cmd_interactive(args):
    print("=== ATOMS Bridge Interactive Shell ===")
    print(f"Target: {args.ip}:{args.port}")
    print("Commands: state, mem gpa|gva <addr> [len], idt [vec], rips [--raw], type <text>, reboot, exit")
    print("-" * 55)

    while True:
        try:
            line = input("bridge> ").strip()
        except (EOFError, KeyboardInterrupt):
            print("\nBye.")
            break

        if not line:
            continue
        if line in ("exit", "quit"):
            break

        if line == "state":
            cmd_state(args)
        elif line.startswith("mem "):
            parts = line.split()
            if len(parts) >= 3:
                args.kind = parts[1]
                args.addr = parts[2]
                args.len = int(parts[3]) if len(parts) > 3 else 64
                cmd_mem(args)
            else:
                print("Usage: mem gpa|gva <addr> [len]")
        elif line.startswith("idt"):
            parts = line.split()
            args.vec = int(parts[1]) if len(parts) > 1 else 0
            cmd_idt(args)
        elif line == "rips":
            args.raw = False
            args.top = 15
            cmd_rips(args)
        elif line == "rips --raw":
            args.raw = True
            args.top = 15
            cmd_rips(args)
        elif line.startswith("type "):
            args.text = [line[5:]]
            cmd_type(args)
        elif line == "reboot":
            cmd_reboot(args)
        else:
            # Generic pass-through
            resp = send_bridge_cmd(line, args.ip, args.port, args.timeout)
            print(resp.strip())


def main():
    parser = argparse.ArgumentParser(description="ATOMS OS Bridge Client")
    parser.add_argument("--ip", default=DEFAULT_IP, help=f"Target IP (default: {DEFAULT_IP})")
    parser.add_argument("--port", type=int, default=DEFAULT_PORT, help=f"Target UDP port (default: {DEFAULT_PORT})")
    parser.add_argument("--timeout", type=float, default=DEFAULT_TIMEOUT, help=f"Socket timeout (default: {DEFAULT_TIMEOUT}s)")
    parser.add_argument("--kernel", default=None, help="Path to FreeBSD kernel.elf for symbol resolution")

    subparsers = parser.add_subparsers(dest="command")

    # state
    subparsers.add_parser("state", help="Dump live guest CPU registers and device state")

    # mem
    mem_p = subparsers.add_parser("mem", help="Dump guest physical or virtual memory")
    mem_p.add_argument("kind", choices=["gpa", "gva"], help="Memory addressing type")
    mem_p.add_argument("addr", help="Memory address (hex or dec)")
    mem_p.add_argument("len", nargs="?", type=int, default=64, help="Byte count (default: 64, max: 256)")

    # idt
    idt_p = subparsers.add_parser("idt", help="Inspect guest IDT entry")
    idt_p.add_argument("vec", nargs="?", type=int, default=0, help="Vector number (0-255)")

    # rips
    rips_p = subparsers.add_parser("rips", help="Dump exit RIPs with symbol resolution & histogram")
    rips_p.add_argument("--raw", action="store_true", help="Print each raw RIP with symbol")
    rips_p.add_argument("--top", type=int, default=15, help="Number of top functions in histogram")

    # type
    type_p = subparsers.add_parser("type", help="Inject text into Virtual UART RX FIFO")
    type_p.add_argument("text", nargs="+", help="Text to inject (escape sequences \\n, \\r supported)")

    # reboot
    subparsers.add_parser("reboot", help="Request guest VM reset")

    # interactive
    subparsers.add_parser("interactive", help="Start interactive command prompt")

    args = parser.parse_args()

    if not args.command:
        # Default to interactive if no arguments
        cmd_interactive(args)
        return 0

    if args.command == "state":
        return cmd_state(args)
    elif args.command == "mem":
        return cmd_mem(args)
    elif args.command == "idt":
        return cmd_idt(args)
    elif args.command == "rips":
        return cmd_rips(args)
    elif args.command == "type":
        return cmd_type(args)
    elif args.command == "reboot":
        return cmd_reboot(args)
    elif args.command == "interactive":
        return cmd_interactive(args)
    else:
        parser.print_help()
        return 1


if __name__ == "__main__":
    sys.exit(main())
