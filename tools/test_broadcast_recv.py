import socket
import select
import time

s_all = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
s_all.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
s_all.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
s_all.bind(("0.0.0.0", 6767))

s_ip = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
s_ip.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
s_ip.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
s_ip.bind(("192.168.2.1", 6767))

sender = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sender.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
sender.bind(("192.168.2.1", 0)) # Send explicitly from Ethernet interface!

print("Testing send to 192.168.2.255...")
sender.sendto(b"TEST_SUBNET_BC", ("192.168.2.255", 6767))

time.sleep(0.1)
r, _, _ = select.select([s_all, s_ip], [], [], 1.0)
print(f"Readable for subnet BC: {[s.getsockname() for s in r]}")
for s in r:
    data, addr = s.recvfrom(1024)
    print(f"Socket {s.getsockname()} got {data} from {addr}")

print("Testing send to 255.255.255.255...")
sender.sendto(b"TEST_GLOBAL_BC", ("255.255.255.255", 6767))
time.sleep(0.1)
r, _, _ = select.select([s_all, s_ip], [], [], 1.0)
print(f"Readable for global BC: {[s.getsockname() for s in r]}")
for s in r:
    data, addr = s.recvfrom(1024)
    print(f"Socket {s.getsockname()} got {data} from {addr}")

s_all.close()
s_ip.close()
sender.close()
