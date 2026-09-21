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
try:
    sys.stdout.reconfigure(encoding='utf-8', errors='replace', line_buffering=True)
except Exception:
    pass

# =====================================================================
# ATOMS OS — PRODUCTION UEFI PXE SERVER (DHCP + TFTP + PROXY-DHCP)
# Host (192.168.2.1) <-> Target PC (192.168.2.100)
# Target MAC: A0:AD:9F:C5:81:27 (ASUS PRIME B760M-K / i3-14100F)
# =====================================================================

BUILD_DIR  = os.path.abspath("build")
SERVER_IP  = "192.168.2.1"
CLIENT_IP  = "192.168.2.100"
NETMASK    = "255.255.255.0"
TARGET_MACS = ["A0:AD:9F:C5:81:27", "E8:65:D4:64:00:69", "C4:A7:2B:B2:8B:41", "08:3C:F4:EE:74:D6", "0A:3C:F4:EE:74:D6"]


class TeeLogger:
    def __init__(self, filepath):
        self.terminal = sys.stdout
        os.makedirs(os.path.dirname(filepath), exist_ok=True)
        self.log = open(filepath, "a", encoding="utf-8", buffering=1, errors='replace')

    def write(self, message):
        try:
            self.terminal.write(message)
            self.terminal.flush()
        except Exception:
            try:
                self.terminal.write(message.encode('ascii', errors='replace').decode('ascii'))
                self.terminal.flush()
            except Exception:
                pass
        try:
            self.log.write(message)
            self.log.flush()
        except Exception:
            pass

    def flush(self):
        try:
            self.terminal.flush()
        except Exception:
            pass
        try:
            self.log.flush()
        except Exception:
            pass

sys.stdout = TeeLogger(os.path.join(BUILD_DIR, "pxe_server.log"))

SIO_UDP_CONNRESET = 0x9800000C

def setup_udp_socket(sock):
    if sys.platform == "win32":
        try:
            sock.ioctl(SIO_UDP_CONNRESET, False)
        except Exception:
            pass
    try:
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, 4 * 1024 * 1024)
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_SNDBUF, 4 * 1024 * 1024)
    except Exception:
        pass

active_tftp_transfers = {} # client_ip -> cancel_event

# --- TFTP SERVER (Port 69) ---
def tftp_worker(client_addr, file_name, options=None, cancel_ev=None):
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
    setup_udp_socket(sock)
    try:
        sock.bind(("0.0.0.0", 0))
    except Exception:
        pass

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
                if cancel_ev and cancel_ev.is_set():
                    return
                try:
                    sock.sendto(oack_resp, client_addr)
                except Exception:
                    pass
                sock.settimeout(1.5)
                try:
                    resp, _ = sock.recvfrom(1024)
                    if len(resp) >= 4:
                        opcode, ack_block = struct.unpack(">HH", resp[:4])
                        if opcode == 4 and ack_block == 0:
                            oack_acked = True
                            break
                except (socket.timeout, ConnectionResetError, OSError):
                    pass
                except Exception:
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
            if cancel_ev and cancel_ev.is_set():
                print(f"\n[TFTP CANCELLED] Transfer to {client_addr} cancelled (superseded).", flush=True)
                return

            data = file_bytes[file_offset:file_offset + block_size]
            file_offset += len(data)
            packet = struct.pack(">HH", 3, block_num) + data # Opcode 3 = DATA

            ack_received = False
            for retry in range(8):
                if cancel_ev and cancel_ev.is_set():
                    return
                try:
                    sock.sendto(packet, client_addr)
                except Exception:
                    pass

                retry_deadline = time.time() + 2.0
                while time.time() < retry_deadline:
                    time_left = max(0.05, retry_deadline - time.time())
                    sock.settimeout(time_left)
                    try:
                        resp, _ = sock.recvfrom(1024)
                        if len(resp) >= 4:
                            opcode, ack_block = struct.unpack(">HH", resp[:4])
                            if opcode == 4 and ack_block == block_num:
                                ack_received = True
                                break
                            elif opcode == 5:
                                err_msg = resp[4:].decode('utf-8', errors='ignore').strip('\x00')
                                print(f"\n[TFTP CLIENT ERROR] {err_msg}", flush=True)
                                return
                    except (socket.timeout, ConnectionResetError, OSError):
                        break
                    except Exception:
                        pass
                if ack_received:
                    break

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
        if cancel_ev and active_tftp_transfers.get(client_addr[0]) == cancel_ev:
            active_tftp_transfers.pop(client_addr[0], None)
        sock.close()

def tftp_server_thread():
    while True:
        try:
            socks = []
            try:
                s1 = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
                setup_udp_socket(s1)
                s1.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
                s1.bind(("0.0.0.0", 69))
                socks.append(s1)
            except Exception as e:
                print(f"[TFTP] 0.0.0.0:69 bind notice: {e}", flush=True)

            try:
                s2 = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
                setup_udp_socket(s2)
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
                        
                        client_ip = addr[0]
                        if client_ip in active_tftp_transfers:
                            print(f"[{datetime.datetime.now().strftime('%H:%M:%S')}] [TFTP] Superseding existing transfer to {client_ip}", flush=True)
                            try:
                                active_tftp_transfers[client_ip].set()
                            except Exception:
                                pass

                        cancel_ev = threading.Event()
                        active_tftp_transfers[client_ip] = cancel_ev
                        threading.Thread(target=tftp_worker, args=(addr, file_name, options, cancel_ev), daemon=True).start()
        except Exception as e:
            print(f"[TFTP CRASH] {e}\n{traceback.format_exc()}", flush=True)
            time.sleep(1)

def is_ethernet_plugged():
    try:
        import ctypes
        from ctypes import wintypes
        class MIB_IF_ROW2(ctypes.Structure):
            _fields_ = [
                ('InterfaceLuid', ctypes.c_uint64),
                ('InterfaceIndex', wintypes.ULONG),
                ('InterfaceGuid', ctypes.c_byte * 16),
                ('Alias', ctypes.c_wchar * 257),
                ('Description', ctypes.c_wchar * 257),
                ('PhysicalAddressLength', wintypes.ULONG),
                ('PhysicalAddress', ctypes.c_ubyte * 32),
                ('PermanentPhysicalAddress', ctypes.c_ubyte * 32),
                ('Mtu', wintypes.ULONG),
                ('Type', wintypes.ULONG),
                ('TunnelType', ctypes.c_int),
                ('MediaType', ctypes.c_int),
                ('PhysicalMediaType', ctypes.c_int),
                ('AccessType', ctypes.c_int),
                ('DirectionType', ctypes.c_int),
                ('InterfaceAndOperStatusFlags', ctypes.c_byte),
                ('OperStatus', ctypes.c_int),
                ('AdminStatus', ctypes.c_int),
                ('MediaConnectState', ctypes.c_int),
                ('NetworkGuid', ctypes.c_byte * 16),
                ('ConnectionType', ctypes.c_int),
            ]
        row = MIB_IF_ROW2()
        row.InterfaceIndex = 4
        if ctypes.windll.iphlpapi.GetIfEntry2(ctypes.byref(row)) == 0:
            return row.MediaConnectState == 1
    except Exception:
        pass
    return True

def detect_network_context(sock=None):
    if is_ethernet_plugged():
        return SERVER_IP, CLIENT_IP, NETMASK, "192.168.2.255", "DIRECT_GBE"

    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.connect(('8.8.8.8', 80))
        wifi_ip = s.getsockname()[0]
        s.close()
        parts = wifi_ip.split('.')
        client_ip = f"{parts[0]}.{parts[1]}.{parts[2]}.199"
        bcast = f"{parts[0]}.{parts[1]}.{parts[2]}.255"
        return wifi_ip, client_ip, "255.255.255.0", bcast, "WIFI_ROUTER"
    except Exception:
        return SERVER_IP, CLIENT_IP, NETMASK, "192.168.2.255", "FALLBACK"

# --- DHCP & PROXY-DHCP SERVER (Ports 67 & 4011) ---
def dhcp_server_thread():
    while True:
        try:
            socks = []
            
            # S1: Wildcard 0.0.0.0:67
            try:
                s_any = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
                setup_udp_socket(s_any)
                s_any.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
                s_any.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
                s_any.bind(("0.0.0.0", 67))
                socks.append(s_any)
            except Exception as e:
                print(f"[DHCP SERVER] 0.0.0.0:67 bind note: {e}", flush=True)

            # S2: Interface IP 192.168.2.1:67
            try:
                s_eth = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
                setup_udp_socket(s_eth)
                s_eth.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
                s_eth.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
                s_eth.bind((SERVER_IP, 67))
                socks.append(s_eth)
            except Exception as e:
                print(f"[DHCP SERVER] {SERVER_IP}:67 bind note: {e}", flush=True)

            # S3: ProxyDHCP port 4011 (BINL for UEFI PXE)
            try:
                s_proxy = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
                setup_udp_socket(s_proxy)
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

            recent_xids = {}

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

                    now_t = time.time()
                    xid_key = (xid, msg_type)
                    if xid_key in recent_xids and (now_t - recent_xids[xid_key]) < 2.0:
                        continue
                    recent_xids[xid_key] = now_t
                    if len(recent_xids) > 100:
                        recent_xids = {k: v for k, v in recent_xids.items() if now_t - v < 10.0}

                    resp_type = 2 if msg_type == 1 else 5 # 2 = DHCPOFFER, 5 = DHCPACK

                    cur_srv_ip, cur_cli_ip, cur_mask, cur_bcast, net_mode = detect_network_context(s)
                    server_ip_bytes = socket.inet_aton(cur_srv_ip)
                    client_ip_bytes = socket.inet_aton(cur_cli_ip)
                    netmask_bytes   = socket.inet_aton(cur_mask)

                    # BOOTP header: op=2 (BOOTREPLY), htype=1, hlen=6, hops=0, xid, secs=0, flags=0x8000 (Broadcast)
                    resp = struct.pack(">BBBBIHH", 2, 1, 6, 0, xid, 0, 0x8000)
                    resp += b'\x00'*4           # ciaddr (0.0.0.0)
                    resp += client_ip_bytes     # yiaddr
                    resp += server_ip_bytes     # siaddr (Next Server for TFTP)
                    resp += b'\x00'*4           # giaddr (0.0.0.0)
                    resp += chaddr_full         # chaddr (16 bytes)

                    sname = cur_srv_ip.encode('ascii')
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
                    targets = [("255.255.255.255", 68), (cur_bcast, 68), (cur_cli_ip, 68)]
                    if addr[0] != "0.0.0.0":
                        targets.append((addr[0], addr[1]))
                    for out_s in socks:
                        for target in targets:
                            try:
                                out_s.sendto(resp, target)
                            except Exception:
                                pass

                    act_out = "OFFER" if resp_type == 2 else "ACK"
                    print(f"[{now_str}] [DHCP {act_out}] Sent to {mac_str} for IP {cur_cli_ip} [{net_mode}] (NextServer: {cur_srv_ip}, BootFile: BOOTX64.EFI)!", flush=True)

        except Exception as e:
            print(f"[DHCP CRASH] {e}\n{traceback.format_exc()}", flush=True)
            time.sleep(1)

# --- UDP LAN DEBUG TELEMETRY SERVER (Port 9999) ---
def udp_debug_server_thread():
    while True:
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            # Disable WSAECONNRESET on Windows so ICMP port unreachable doesn't crash recvfrom
            if sys.platform == "win32":
                try:
                    SIO_UDP_CONNRESET = 0x9800000C
                    sock.ioctl(SIO_UDP_CONNRESET, False)
                except Exception:
                    pass
            sock.bind(("0.0.0.0", 9999))
            print(f"[LAN DEBUG SERVER] Listening on UDP 0.0.0.0:9999...", flush=True)
            log_path = os.path.join(BUILD_DIR, "atoms_live_kernel.log")
            while True:
                try:
                    data, addr = sock.recvfrom(4096)
                except ConnectionResetError:
                    continue
                except Exception:
                    continue
                msg = data.decode('utf-8', errors='replace').strip()
                try:
                    print(f"[KERNEL TELEMETRY] {msg}", flush=True)
                except Exception:
                    pass
                try:
                    with open(log_path, "a", encoding="utf-8", errors='replace') as f_log:
                        f_log.write(f"[{datetime.datetime.now().strftime('%H:%M:%S.%f')[:-3]}] {msg}\n")
                except Exception:
                    pass
        except Exception as e:
            try:
                print(f"[LAN DEBUG CRASH] {e}", flush=True)
            except Exception:
                pass
            time.sleep(1)

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
