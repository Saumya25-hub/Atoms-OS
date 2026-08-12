import socket
import struct
import os
import sys
import threading
import time

sys.stdout.reconfigure(line_buffering=True)

# =====================================================================
# ATOMS OS — AUTOMATED NATIVE PYTHON PXE SERVER (DHCP + TFTP)
# Direct Laptop (192.168.2.1) <-> H81 Board (192.168.2.100)
# =====================================================================

BUILD_DIR = os.path.abspath("build")
SERVER_IP = "192.168.2.1"
CLIENT_IP = "192.168.2.100"
NETMASK   = "255.255.255.0"

# --- TFTP SERVER (Port 69) ---
def tftp_worker(client_addr, file_name, options=None):
    file_path = os.path.join(BUILD_DIR, os.path.basename(file_name))
    print(f"\n[TFTP] Client {client_addr} requested file: '{file_name}' -> '{file_path}'")
    
    if not os.path.exists(file_path):
        print(f"[TFTP ERROR] File '{file_path}' NOT FOUND!")
        return

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind((SERVER_IP, 0)) # Ephemeral port for TFTP transfer
    
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
            # Send OACK and wait for ACK 0
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
            block_num = 1
            while True:
                data = f.read(block_size)
                packet = struct.pack(">HH", 3, block_num) + data # Opcode 3 = DATA
                
                # Send DATA packet with retries
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
        print(f"[TFTP SUCCESS] Sent '{file_name}' to {client_addr} 100%!")
    except Exception as e:
        print(f"[TFTP EXCEPTION] {e}")
    finally:
        sock.close()

def tftp_server_thread():
    while True:
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            sock.bind(("0.0.0.0", 69))
            print(f"[TFTP SERVER] Active on 0.0.0.0:69 serving '{BUILD_DIR}'", flush=True)
            
            while True:
                try:
                    data, addr = sock.recvfrom(2048)
                    print(f"[TFTP RAW] Received {len(data)} bytes from {addr}", flush=True)
                    if len(data) > 2 and data[0] == 0 and data[1] == 1: # Opcode 1 = RRQ (Read Request)
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
                except Exception as e:
                    time.sleep(0.05)
        except Exception as e:
            print(f"[TFTP CRASH] {e}\n{traceback.format_exc()}", flush=True)
            time.sleep(1)

# --- DHCP SERVER (Port 67) ---
import traceback

def dhcp_server_thread():
    while True:
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
            try:
                sock.bind((SERVER_IP, 67))
                print(f"[DHCP SERVER] Active on {SERVER_IP}:67 offering IP {CLIENT_IP} to H81 PXE", flush=True)
            except Exception as e:
                sock.bind(("0.0.0.0", 67))
                print(f"[DHCP SERVER] Active on 0.0.0.0:67 offering IP {CLIENT_IP} to H81 PXE (Fallback: {e})", flush=True)

            server_ip_bytes = socket.inet_aton(SERVER_IP)
            client_ip_bytes = socket.inet_aton(CLIENT_IP)
            netmask_bytes   = socket.inet_aton(NETMASK)

            while True:
                try:
                    data, addr = sock.recvfrom(1024)
                    if len(data) < 240:
                        continue

                    op, htype, hlen, hops, xid, secs, flags = struct.unpack(">BBBBIHH", data[:12])
                    chaddr = data[28:34]

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
                    print(f"[DHCP REQUEST] Received from MAC {mac_str} (MsgType: {msg_type})", flush=True)

                    resp_type = 2 if msg_type == 1 else 5
                    
                    resp = struct.pack(">BBBBIHH", 2, 1, 6, 0, xid, 0, flags)
                    resp += b'\x00'*4
                    resp += client_ip_bytes
                    resp += server_ip_bytes
                    resp += b'\x00'*4
                    resp += chaddr + b'\x00'*10
                    resp += b'192.168.2.1' + b'\x00'*(64 - 11)
                    resp += b'BOOTX64.EFI' + b'\x00'*(128 - 11)
                    resp += b'\x63\x82\x53\x63'

                    tftp_name = b"192.168.2.1\x00"
                    resp += struct.pack(">BBB", 53, 1, resp_type)
                    resp += struct.pack(">BB", 1, 4) + netmask_bytes
                    resp += struct.pack(">BB", 3, 4) + server_ip_bytes
                    resp += struct.pack(">BB", 54, 4) + server_ip_bytes
                    resp += struct.pack(">BB", 66, len(tftp_name)) + tftp_name
                    resp += struct.pack(">BB", 67, 12) + b'BOOTX64.EFI\x00'
                    resp += b'\xFF'

                    for target in [("255.255.255.255", 68), ("192.168.2.255", 68), (CLIENT_IP, 68)]:
                        try:
                            sock.sendto(resp, target)
                        except Exception:
                            pass
                    print(f"[DHCP RESPOND] Sent DHCP OFFER/ACK (type {resp_type}) for {CLIENT_IP} to {mac_str}!", flush=True)

                except Exception as e:
                    time.sleep(0.05)
        except Exception as e:
            print(f"[DHCP CRASH] {e}\n{traceback.format_exc()}", flush=True)
            time.sleep(1)

if __name__ == "__main__":
    print("=" * 65, flush=True)
    print("  ATOMS OS — AUTOMATED NATIVE PYTHON PXE SERVER", flush=True)
    print("=================================================================", flush=True)
    print(f" Server IP: {SERVER_IP} | Target H81 IP: {CLIENT_IP}", flush=True)
    print(f" Build Folder: {BUILD_DIR}", flush=True)
    print("=================================================================", flush=True)
    
    t1 = threading.Thread(target=dhcp_server_thread, daemon=True)
    t2 = threading.Thread(target=tftp_server_thread, daemon=True)
    t1.start()
    t2.start()

    while True:
        try:
            time.sleep(100)
        except BaseException as e:
            time.sleep(1)
