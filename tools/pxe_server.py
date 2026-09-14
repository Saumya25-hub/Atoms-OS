import socket
import struct
import os
import sys
import threading
import time
import datetime
import traceback
import select

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
sys.stdout.reconfigure(line_buffering=True)

# =====================================================================
# ATOMS OS — PRODUCTION UEFI PXE SERVER (DHCP + TFTP + PROXY-DHCP)
# Host (192.168.2.1) <-> Target PC (192.168.2.100)
# Target MAC: A0:AD:9F:C5:81:27 (ASUS PRIME B760M-K / i3-14100F)
# =====================================================================

BUILD_DIR  = os.path.abspath("build")
SERVER_IP  = "192.168.2.1"
CLIENT_IP  = "192.168.2.100"
NETMASK    = "255.255.255.0"
TARGET_MAC = "A0:AD:9F:C5:81:27"
TARGET_MACS = ["A0:AD:9F:C5:81:27", "08:3C:F4:EE:74:D6", "0A:3C:F4:EE:74:D6"]

class TeeLogger:
    def __init__(self, filepath):
        self.terminal = sys.stdout
        os.makedirs(os.path.dirname(filepath), exist_ok=True)
        self.log = open(filepath, "a", encoding="utf-8", buffering=1)

    def write(self, message):
        self.terminal.write(message)
        self.terminal.flush()
        try:
            self.log.write(message)
            self.log.flush()
        except Exception:
            pass

    def flush(self):
        self.terminal.flush()
        try:
            self.log.flush()
        except Exception:
            pass

sys.stdout = TeeLogger(os.path.join(BUILD_DIR, "pxe_server.log"))

# --- TFTP SERVER (Port 69) ---
def tftp_worker(client_addr, file_name, options=None):
    file_basename = os.path.basename(file_name)
    file_path = os.path.join(BUILD_DIR, file_basename)
    if not os.path.exists(file_path):
        for item in os.listdir(BUILD_DIR):
            if item.lower() == file_basename.lower():
                file_path = os.path.join(BUILD_DIR, item)
                break

    now_str = datetime.datetime.now().strftime('%H:%M:%S')
    print(f"\n[{now_str}] [TFTP REQUEST] Client {client_addr} requested: '{file_name}' -> '{file_path}'", flush=True)

    if not os.path.exists(file_path):
        print(f"[{now_str}] [TFTP ERROR] File '{file_path}' NOT FOUND in '{BUILD_DIR}'!", flush=True)
        return

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        sock.bind((SERVER_IP, 0))
    except Exception:
        sock.bind(("0.0.0.0", 0))

    try:
        file_size = os.path.getsize(file_path)
        block_size = 512
        oack_resp = bytearray(struct.pack(">H", 6)) # Opcode 6 = OACK
        send_oack = False

        if options:
            for k, v in options.items():
                k_lower = k.lower()
                if k_lower == "blksize":
                    block_size = min(int(v), 1468)
                    oack_resp += b"blksize\x00" + str(block_size).encode() + b"\x00"
                    send_oack = True
                elif k_lower == "tsize":
                    oack_resp += b"tsize\x00" + str(file_size).encode() + b"\x00"
                    send_oack = True

        if send_oack:
            oack_acked = False
            for _ in range(5):
                sock.sendto(oack_resp, client_addr)
                sock.settimeout(1.5)
                try:
                    resp, _ = sock.recvfrom(1024)
                    opcode, ack_block = struct.unpack(">HH", resp[:4])
                    if opcode == 4 and ack_block == 0:
                        oack_acked = True
                        break
                except socket.timeout:
                    pass
            if not oack_acked:
                print(f"[{now_str}] [TFTP WARNING] Client did not ACK OACK, proceeding with transfer...", flush=True)

        with open(file_path, "rb") as f:
            file_bytes = f.read()

        file_offset = 0
        block_num = 1
        total_blocks = (len(file_bytes) + block_size - 1) // block_size
        t_start = time.time()
        print(f"[{now_str}] [TFTP START] Sending '{os.path.basename(file_path)}' ({len(file_bytes):,} bytes, {total_blocks} blocks of {block_size}B) to {client_addr}...", flush=True)

        last_progress_time = time.time()
        while True:
            data = file_bytes[file_offset:file_offset + block_size]
            file_offset += len(data)
            packet = struct.pack(">HH", 3, block_num) + data # Opcode 3 = DATA

            ack_received = False
            for _ in range(5):
                sock.sendto(packet, client_addr)
                sock.settimeout(2.0)
                try:
                    resp, _ = sock.recvfrom(1024)
                    opcode, ack_block = struct.unpack(">HH", resp[:4])
                    if opcode == 4 and ack_block == block_num:
                        ack_received = True
                        break
                except socket.timeout:
                    pass

            if not ack_received:
                print(f"\n[TFTP TIMEOUT] Client {client_addr} failed to ACK block {block_num}/{total_blocks}! Aborting.", flush=True)
                break

            now = time.time()
            if now - last_progress_time >= 1.0 or block_num == total_blocks:
                pct = int((block_num / max(1, total_blocks)) * 100)
                elapsed = max(0.001, now - t_start)
                speed_mb = (file_offset / (1024 * 1024)) / elapsed
                print(f"\r[TFTP PROGRESS] Block {block_num}/{total_blocks} ({pct}%) — {speed_mb:.2f} MB/s", end="", flush=True)
                last_progress_time = now

            block_num = (block_num + 1) & 0xFFFF
            if len(data) < block_size:
                break

        elapsed = max(0.001, time.time() - t_start)
        speed_mb = (len(file_bytes) / (1024 * 1024)) / elapsed
        print(f"\n[{datetime.datetime.now().strftime('%H:%M:%S')}] [TFTP SUCCESS] Sent '{os.path.basename(file_path)}' ({len(file_bytes):,} bytes) in {elapsed:.2f}s ({speed_mb:.2f} MB/s) to {client_addr} 100%!", flush=True)
    except Exception as e:
        print(f"\n[TFTP EXCEPTION] {e}\n{traceback.format_exc()}", flush=True)
    finally:
        sock.close()

def tftp_server_thread():
    while True:
        try:
            socks = []
            try:
                s1 = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
                s1.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
                s1.bind(("0.0.0.0", 69))
                socks.append(s1)
            except Exception as e:
                print(f"[TFTP] 0.0.0.0:69 bind notice: {e}", flush=True)

            try:
                s2 = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
                s2.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
                s2.bind((SERVER_IP, 69))
                socks.append(s2)
            except Exception as e:
                print(f"[TFTP] {SERVER_IP}:69 bind notice: {e}", flush=True)

            if not socks:
                time.sleep(2)
                continue

            print(f"[TFTP SERVER] Active on port 69 serving '{BUILD_DIR}'", flush=True)

            while True:
                r, _, _ = select.select(socks, [], [], 0.5)
                for s in r:
                    try:
                        data, addr = s.recvfrom(2048)
                    except Exception:
                        continue

                    if len(data) > 2 and data[0] == 0 and data[1] == 1: # Opcode 1 = RRQ
                        parts = data[2:].split(b'\x00')
                        file_name = parts[0].decode('utf-8', errors='ignore').replace('\\', '/').strip('\x00').strip()
                        options = {}
                        i = 2
                        while i + 1 < len(parts):
                            k = parts[i].decode('utf-8', errors='ignore').strip('\x00').strip()
                            v = parts[i+1].decode('utf-8', errors='ignore').strip('\x00').strip()
                            if k:
                                options[k] = v
                            i += 2
                        threading.Thread(target=tftp_worker, args=(addr, file_name, options), daemon=True).start()
        except Exception as e:
            print(f"[TFTP CRASH] {e}\n{traceback.format_exc()}", flush=True)
            time.sleep(1)

# --- DHCP & PROXY-DHCP SERVER (Ports 67 & 4011) ---
def dhcp_server_thread():
    while True:
        try:
            socks = []
            
            # S1: Wildcard 0.0.0.0:67
            try:
                s_any = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
                s_any.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
                s_any.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
                s_any.bind(("0.0.0.0", 67))
                socks.append(s_any)
            except Exception as e:
                print(f"[DHCP SERVER] 0.0.0.0:67 bind note: {e}", flush=True)

            # S2: Interface IP 192.168.2.1:67
            try:
                s_eth = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
                s_eth.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
                s_eth.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
                s_eth.bind((SERVER_IP, 67))
                socks.append(s_eth)
            except Exception as e:
                print(f"[DHCP SERVER] {SERVER_IP}:67 bind note: {e}", flush=True)

            # S3: ProxyDHCP port 4011 (BINL for UEFI PXE)
            try:
                s_proxy = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
                s_proxy.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
                s_proxy.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
                s_proxy.bind(("0.0.0.0", 4011))
                socks.append(s_proxy)
            except Exception as e:
                pass

            if not socks:
                time.sleep(2)
                continue

            print(f"[DHCP SERVER] Listening on ports 67 & 4011 across {[s.getsockname() for s in socks]}", flush=True)

            server_ip_bytes = socket.inet_aton(SERVER_IP)
            client_ip_bytes = socket.inet_aton(CLIENT_IP)
            netmask_bytes   = socket.inet_aton(NETMASK)

            while True:
                r, _, _ = select.select(socks, [], [], 0.5)
                for s in r:
                    try:
                        data, addr = s.recvfrom(2048)
                    except Exception:
                        continue

                    now_str = datetime.datetime.now().strftime('%H:%M:%S.%f')[:-3]
                    print(f"[{now_str}] [RAW PACKET] {len(data)} bytes received from {addr} on {s.getsockname()}", flush=True)

                    if len(data) < 240:
                        continue

                    if data[236:240] != b'\x63\x82\x53\x63':
                        continue

                    op, htype, hlen, hops, xid, secs, flags = struct.unpack(">BBBBIHH", data[:12])
                    chaddr_full = data[28:44]
                    chaddr = data[28:34]
                    mac_str = ":".join(f"{b:02X}" for b in chaddr)

                    msg_type = 1
                    client_uuid = None
                    arch_type = None
                    idx = 240
                    while idx < len(data):
                        opt = data[idx]
                        if opt == 255: break
                        if opt == 0: idx += 1; continue
                        l = data[idx+1]
                        opt_data = data[idx+2 : idx+2+l]
                        if opt == 53 and l >= 1:
                            msg_type = opt_data[0]
                        elif opt == 97:
                            client_uuid = opt_data
                        elif opt == 93 and l >= 2:
                            arch_type = struct.unpack(">H", opt_data[:2])[0]
                        idx += 2 + l

                    arch_str = f"x64 UEFI (7)" if arch_type == 7 else f"{arch_type}"
                    act_in = "DISCOVER" if msg_type == 1 else ("REQUEST" if msg_type == 3 else f"TYPE_{msg_type}")
                    print(f"[{now_str}] [DHCP {act_in}] From MAC {mac_str} (Arch: {arch_str}) via {addr}", flush=True)

                    resp_type = 2 if msg_type == 1 else 5 # 2 = DHCPOFFER, 5 = DHCPACK

                    # BOOTP header: op=2 (BOOTREPLY), htype=1, hlen=6, hops=0, xid, secs=0, flags=0x8000 (Broadcast)
                    resp = struct.pack(">BBBBIHH", 2, 1, 6, 0, xid, 0, 0x8000)
                    resp += b'\x00'*4           # ciaddr (0.0.0.0)
                    resp += client_ip_bytes     # yiaddr (192.168.2.100)
                    resp += server_ip_bytes     # siaddr (192.168.2.1 - Next Server for TFTP)
                    resp += b'\x00'*4           # giaddr (0.0.0.0)
                    resp += chaddr_full         # chaddr (16 bytes)

                    sname = SERVER_IP.encode('ascii')
                    resp += sname + b'\x00' * (64 - len(sname))

                    boot_file = b'BOOTX64.EFI'
                    resp += boot_file + b'\x00' * (128 - len(boot_file))

                    resp += b'\x63\x82\x53\x63' # Magic Cookie

                    # DHCP Options:
                    resp += struct.pack(">BBB", 53, 1, resp_type)          # Option 53: Message Type
                    resp += struct.pack(">BB", 54, 4) + server_ip_bytes    # Option 54: Server Identifier
                    resp += struct.pack(">BBI", 51, 4, 86400)              # Option 51: Lease Time (86400s)
                    resp += struct.pack(">BB", 1, 4) + netmask_bytes       # Option 1: Subnet Mask
                    resp += struct.pack(">BB", 3, 4) + server_ip_bytes     # Option 3: Router
                    resp += struct.pack(">BB", 6, 4) + server_ip_bytes     # Option 6: DNS

                    # Option 60: Vendor Class Identifier = "PXEClient" (MANDATORY FOR UEFI PXE)
                    pxe_client = b"PXEClient"
                    resp += struct.pack(">BB", 60, len(pxe_client)) + pxe_client

                    # Option 66: TFTP Server Name (no trailing null)
                    resp += struct.pack(">BB", 66, len(sname)) + sname

                    # Option 67: Bootfile Name (no trailing null)
                    resp += struct.pack(">BB", 67, len(boot_file)) + boot_file

                    # Option 97: Client UUID (echo back if client sent it)
                    if client_uuid:
                        resp += struct.pack(">BB", 97, len(client_uuid)) + client_uuid

                    # Option 43: PXE Vendor-Specific Options (Disable Multicast Discovery Prompt)
                    opt43 = b"\x06\x01\x08\xFF"
                    resp += struct.pack(">BB", 43, len(opt43)) + opt43

                    resp += b'\xFF' # Option 255: End

                    # Transmit reply via all available sockets to reach the physical wire
                    targets = [("255.255.255.255", 68), ("192.168.2.255", 68), (CLIENT_IP, 68), (addr[0], addr[1])]
                    for out_s in socks:
                        for target in targets:
                            try:
                                out_s.sendto(resp, target)
                            except Exception:
                                pass

                    act_out = "OFFER" if resp_type == 2 else "ACK"
                    print(f"[{now_str}] [DHCP {act_out}] Sent to {mac_str} for IP {CLIENT_IP} (NextServer: {SERVER_IP}, BootFile: BOOTX64.EFI)!", flush=True)

        except Exception as e:
            print(f"[DHCP CRASH] {e}\n{traceback.format_exc()}", flush=True)
            time.sleep(1)

# --- UDP LAN DEBUG TELEMETRY SERVER (Port 9999) ---
def udp_debug_server_thread():
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        sock.bind(("0.0.0.0", 9999))
        print(f"[LAN DEBUG SERVER] Listening on UDP 0.0.0.0:9999...", flush=True)
        log_path = os.path.join(BUILD_DIR, "atoms_live_kernel.log")
        while True:
            data, addr = sock.recvfrom(4096)
            msg = data.decode('utf-8', errors='ignore').strip()
            print(f"[KERNEL TELEMETRY] {msg}", flush=True)
            try:
                with open(log_path, "a", encoding="utf-8") as f_log:
                    f_log.write(f"[{datetime.datetime.now().strftime('%H:%M:%S.%f')[:-3]}] {msg}\n")
            except Exception:
                pass
    except Exception as e:
        print(f"[LAN DEBUG CRASH] {e}", flush=True)

# --- UDP SCREENSHOT RECEIVER (Port 9998) ---
def udp_screenshot_server_thread():
    try:
        from forensic_test_controller import run_screenshot_listener
        run_screenshot_listener()
    except Exception as e:
        print(f"[SCREENSHOT SERVER CRASH] {e}", flush=True)

def send_wol(mac=None):
    macs = [mac] if mac else TARGET_MACS
    for m in macs:
        try:
            clean_mac = m.replace(":", "").replace("-", "")
            mac_bytes = bytes.fromhex(clean_mac)
            magic_packet = b"\xff" * 6 + mac_bytes * 16
            with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as s:
                s.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
                try:
                    s.bind((SERVER_IP, 0))
                except Exception:
                    pass
                for dest in ["255.255.255.255", "192.168.2.255"]:
                    try:
                        s.sendto(magic_packet, (dest, 9))
                        s.sendto(magic_packet, (dest, 7))
                    except Exception:
                        pass
            print(f"[{datetime.datetime.now().strftime('%H:%M:%S')}] [WOL] Broadcast Wake-On-LAN packet to MAC {m}", flush=True)
        except Exception as e:
            print(f"[WOL ERROR] ({m}): {e}", flush=True)

if __name__ == "__main__":
    print("=" * 65)
    print("  ATOMS OS — PRODUCTION UEFI PXE SERVER (FINAL)")
    print("=" * 65)
    print(f" Server IP:   {SERVER_IP}")
    print(f" Target IP:   {CLIENT_IP}")
    print(f" Target MACs: {', '.join(TARGET_MACS)}")
    print(f" Boot File:   BOOTX64.EFI")
    print(f" Build Dir:   {BUILD_DIR}")
    print("=" * 65)

    # Initialize / clean live kernel log
    log_path = os.path.join(BUILD_DIR, "atoms_live_kernel.log")
    try:
        with open(log_path, "w", encoding="utf-8") as f_log:
            f_log.write(f"=== ATOMS OS REAL HARDWARE PXE SESSION STARTED {datetime.datetime.now()} ===\n")
    except Exception:
        pass

    t_dhcp = threading.Thread(target=dhcp_server_thread, daemon=True)
    t_tftp = threading.Thread(target=tftp_server_thread, daemon=True)
    t_dbg  = threading.Thread(target=udp_debug_server_thread, daemon=True)
    t_ss   = threading.Thread(target=udp_screenshot_server_thread, daemon=True)

    t_dhcp.start()
    t_tftp.start()
    t_dbg.start()
    t_ss.start()

    # Initial Wake-On-LAN
    send_wol()

    loop_count = 0
    while True:
        time.sleep(1)
        loop_count += 1
        # Broadcast WOL every 15s for the first 2 minutes
        if loop_count in (15, 30, 45, 60, 75, 90, 105, 120):
            send_wol()
