import socket
import struct
import os
import sys
import threading
import time
import datetime
import traceback

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
sys.stdout.reconfigure(line_buffering=True)

# =====================================================================
# ATOMS OS — PRODUCTION UEFI PXE SERVER (DHCP + TFTP)
# Host (192.168.2.1) <-> Asus B750MK / H81 Target (192.168.2.100)
# =====================================================================

BUILD_DIR = os.path.abspath("build")
SERVER_IP = "192.168.2.1"
CLIENT_IP = "192.168.2.100"
NETMASK   = "255.255.255.0"

# --- TFTP SERVER (Port 69) ---
def tftp_worker(client_addr, file_name, options=None):
    file_basename = os.path.basename(file_name)
    file_path = os.path.join(BUILD_DIR, file_basename)
    if not os.path.exists(file_path):
        for item in os.listdir(BUILD_DIR):
            if item.lower() == file_basename.lower():
                file_path = os.path.join(BUILD_DIR, item)
                break

    print(f"\n[TFTP REQUEST] Client {client_addr} requested: '{file_name}' -> '{file_path}'", flush=True)
    
    if not os.path.exists(file_path):
        print(f"[TFTP ERROR] File '{file_path}' NOT FOUND in '{BUILD_DIR}'!", flush=True)
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
                    block_size = int(v)
                    oack_resp += b"blksize\x00" + str(block_size).encode() + b"\x00"
                    send_oack = True
                elif k_lower == "tsize":
                    oack_resp += b"tsize\x00" + str(file_size).encode() + b"\x00"
                    send_oack = True

        if send_oack:
            for _ in range(5):
                sock.sendto(oack_resp, client_addr)
                sock.settimeout(2.0)
                try:
                    resp, _ = sock.recvfrom(1024)
                    opcode, ack_block = struct.unpack(">HH", resp[:4])
                    if opcode == 4 and ack_block == 0:
                        break
                except socket.timeout:
                    pass

        with open(file_path, "rb") as f:
            file_bytes = f.read()

        file_offset = 0
        block_num = 1
        print(f"[TFTP START] Sending '{file_path}' ({len(file_bytes)} bytes) to {client_addr}...", flush=True)
        while True:
            data = file_bytes[file_offset:file_offset + block_size]
            file_offset += len(data)
            packet = struct.pack(">HH", 3, block_num) + data # Opcode 3 = DATA
            
            for _ in range(5):
                sock.sendto(packet, client_addr)
                sock.settimeout(2.0)
                try:
                    resp, _ = sock.recvfrom(1024)
                    opcode, ack_block = struct.unpack(">HH", resp[:4])
                    if opcode == 4 and ack_block == block_num: # Opcode 4 = ACK
                        break
                except socket.timeout:
                    pass
            
            block_num = (block_num + 1) & 0xFFFF
            if len(data) < block_size:
                break
        print(f"[TFTP SUCCESS] Sent '{file_name}' ({len(file_bytes)} bytes) to {client_addr} 100%!", flush=True)
    except Exception as e:
        print(f"[TFTP EXCEPTION] {e}\n{traceback.format_exc()}", flush=True)
    finally:
        sock.close()

def tftp_server_thread():
    while True:
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            sock.bind((SERVER_IP, 69))
            print(f"[TFTP SERVER] Active on {SERVER_IP}:69 serving '{BUILD_DIR}'", flush=True)
            
            while True:
                try:
                    data, addr = sock.recvfrom(2048)
                    print(f"[TFTP RAW] Received {len(data)} bytes from {addr}", flush=True)
                    if len(data) > 2 and data[0] == 0 and data[1] == 1: # Opcode 1 = RRQ
                        parts = data[2:].split(b'\x00')
                        file_name = parts[0].decode('utf-8', errors='ignore').replace('\\', '/')
                        options = {}
                        i = 2
                        while i + 1 < len(parts):
                            k = parts[i].decode('utf-8', errors='ignore')
                            v = parts[i+1].decode('utf-8', errors='ignore')
                            if k:
                                options[k] = v
                            i += 2
                        threading.Thread(target=tftp_worker, args=(addr, file_name, options), daemon=True).start()
                except Exception:
                    time.sleep(0.05)
        except Exception as e:
            print(f"[TFTP CRASH] {e}\n{traceback.format_exc()}", flush=True)
            time.sleep(1)

# --- DHCP SERVER (Port 67) ---
def dhcp_server_thread():
    while True:
        try:
            # Listening socket on 0.0.0.0:67
            recv_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            recv_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            recv_sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
            recv_sock.bind(("0.0.0.0", 67))

            # Dedicated sender socket bound to port 67 on SERVER_IP (192.168.2.1)
            # This ensures source IP is 192.168.2.1 AND source port is strictly 67!
            send_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            send_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            send_sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
            send_sock.bind((SERVER_IP, 67))

            print(f"[DHCP SERVER] Listening on 0.0.0.0:67 & Sending from {SERVER_IP}:67 -> Target {CLIENT_IP}", flush=True)

            server_ip_bytes = socket.inet_aton(SERVER_IP)
            client_ip_bytes = socket.inet_aton(CLIENT_IP)
            netmask_bytes   = socket.inet_aton(NETMASK)

            while True:
                try:
                    data, addr = recv_sock.recvfrom(2048)
                    if len(data) < 240:
                        continue

                    op, htype, hlen, hops, xid, secs, flags = struct.unpack(">BBBBIHH", data[:12])
                    chaddr = data[28:34]

                    # Check Magic Cookie
                    if data[236:240] != b'\x63\x82\x53\x63':
                        continue

                    msg_type = 1
                    idx = 240
                    while idx < len(data):
                        opt = data[idx]
                        if opt == 255: break
                        if opt == 0: idx += 1; continue
                        l = data[idx+1]
                        if opt == 53: msg_type = data[idx+2]
                        idx += 2 + l

                    mac_str = ":".join(f"{b:02X}" for b in chaddr)
                    print(f"[DHCP REQUEST] Received from MAC {mac_str} (MsgType: {msg_type}) from {addr}", flush=True)

                    resp_type = 2 if msg_type == 1 else 5 # 2 = DHCPOFFER, 5 = DHCPACK
                    
                    # Force broadcast flag (0x8000) so client receives it before assigning IP
                    flags_out = 0x8000

                    # BOOTP header: op=2 (BOOTREPLY), htype=1, hlen=6, hops=0, xid, secs=0, flags=0x8000
                    resp = struct.pack(">BBBBIHH", 2, 1, 6, 0, xid, 0, flags_out)
                    resp += b'\x00'*4           # ciaddr (0.0.0.0)
                    resp += client_ip_bytes     # yiaddr (192.168.2.100)
                    resp += server_ip_bytes     # siaddr (192.168.2.1 - Next Server for TFTP)
                    resp += b'\x00'*4           # giaddr (0.0.0.0)
                    resp += chaddr + b'\x00'*10 # chaddr (16 bytes)
                    
                    sname = b'192.168.2.1'
                    resp += sname + b'\x00' * (64 - len(sname))
                    
                    boot_file = b'BOOTX64.EFI'
                    resp += boot_file + b'\x00' * (128 - len(boot_file))
                    
                    resp += b'\x63\x82\x53\x63' # Magic Cookie

                    # DHCP Options:
                    resp += struct.pack(">BBB", 53, 1, resp_type)          # Option 53: Message Type
                    resp += struct.pack(">BB", 54, 4) + server_ip_bytes    # Option 54: Server Identifier
                    resp += struct.pack(">BBI", 51, 4, 86400)              # Option 51: Lease Time (86400s) - MANDATORY!
                    resp += struct.pack(">BB", 1, 4) + netmask_bytes       # Option 1: Subnet Mask
                    resp += struct.pack(">BB", 3, 4) + server_ip_bytes     # Option 3: Router
                    
                    # Option 60: Vendor Class Identifier = "PXEClient" (MANDATORY FOR UEFI PXE)
                    pxe_client = b"PXEClient"
                    resp += struct.pack(">BB", 60, len(pxe_client)) + pxe_client

                    # Option 66: TFTP Server Name
                    tftp_name = b"192.168.2.1\x00"
                    resp += struct.pack(">BB", 66, len(tftp_name)) + tftp_name
                    
                    # Option 67: Bootfile Name
                    resp += struct.pack(">BB", 67, 12) + b'BOOTX64.EFI\x00'
                    
                    # Option 43: PXE Vendor-Specific Options (Disable Multicast Discovery Prompt)
                    opt43 = b"\x06\x01\x08\xFF"
                    resp += struct.pack(">BB", 43, len(opt43)) + opt43

                    resp += b'\xFF' # Option 255: End

                    # Send reply strictly via 192.168.2.1:67
                    for target in [("255.255.255.255", 68), ("192.168.2.255", 68), (CLIENT_IP, 68)]:
                        try:
                            send_sock.sendto(resp, target)
                        except Exception:
                            pass

                    print(f"[DHCP RESPOND] Sent DHCP OFFER/ACK (type {resp_type}) from {SERVER_IP}:67 to {mac_str}!", flush=True)

                except Exception as e:
                    time.sleep(0.05)
        except Exception as e:
            print(f"[DHCP CRASH] {e}\n{traceback.format_exc()}", flush=True)
            time.sleep(1)

# --- UDP LAN DEBUG TELEMETRY SERVER (Port 9999) ---
def udp_debug_server_thread():
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        try:
            sock.bind((SERVER_IP, 9999))
        except Exception:
            sock.bind(("0.0.0.0", 9999))
        print(f"[LAN DEBUG SERVER] Listening on UDP 9999...", flush=True)
        log_path = os.path.join(BUILD_DIR, "atoms_live_kernel.log")
        while True:
            data, addr = sock.recvfrom(4096)
            msg = data.decode('utf-8', errors='ignore').strip()
            print(f"[H81 TELEMETRY] {msg}", flush=True)
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

if __name__ == "__main__":
    print("=" * 65)
    print("  ATOMS OS — PRODUCTION UEFI PXE SERVER")
    print("=" * 65)
    print(f" Server IP: {SERVER_IP} | Target H81 IP: {CLIENT_IP}")
    print(f" Build Folder: {BUILD_DIR}")
    print("=" * 65)

    t_dhcp = threading.Thread(target=dhcp_server_thread, daemon=True)
    t_tftp = threading.Thread(target=tftp_server_thread, daemon=True)
    t_dbg  = threading.Thread(target=udp_debug_server_thread, daemon=True)
    t_ss   = threading.Thread(target=udp_screenshot_server_thread, daemon=True)

    t_dhcp.start()
    t_tftp.start()
    t_dbg.start()
    t_ss.start()

    while True:
        time.sleep(1)
