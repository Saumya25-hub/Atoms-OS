import socket
import struct
import time

try:
    # Create raw socket on Windows
    s = socket.socket(socket.AF_INET, socket.SOCK_RAW, socket.IPPROTO_IP)
    s.bind(("192.168.2.1", 0))
    s.setsockopt(socket.IPPROTO_IP, socket.IP_HDRINCL, 1)
    # Enable promiscuous mode
    SIO_RCVALL = 0x98000001
    s.ioctl(SIO_RCVALL, 1)
    print("[RAW SNIFFER] SIO_RCVALL enabled on 192.168.2.1! Listening for 5 seconds...")
    s.settimeout(1.0)
    t_end = time.time() + 5.0
    count = 0
    while time.time() < t_end:
        try:
            data, addr = s.recvfrom(65535)
            count += 1
            # IP header
            proto = data[9]
            src_ip = socket.inet_ntoa(data[12:16])
            dst_ip = socket.inet_ntoa(data[16:20])
            print(f"[PACKET #{count}] Proto: {proto} | {src_ip} -> {dst_ip} ({len(data)} bytes)")
            if proto == 17: # UDP
                src_port, dst_port = struct.unpack(">HH", data[20:24])
                print(f"   UDP: {src_port} -> {dst_port}")
        except socket.timeout:
            pass
    print(f"[RAW SNIFFER] Done. Captured {count} packets.")
except Exception as e:
    print(f"[RAW SNIFFER ERROR] {e}")
