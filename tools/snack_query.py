import socket
import sys
import time

s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
try:
    s.bind(('0.0.0.0', 9999))
except Exception as e:
    print(f"Bind error: {e}")
    sys.exit(1)

s.settimeout(2.5)
msg = b"JOB:FORENSIC_VTNET_ATTACH\n"
s.sendto(msg, ('192.168.2.50', 9999))
s.sendto(msg, ('192.168.2.255', 9999))
print("Sent JOB:FORENSIC_VTNET_ATTACH to 192.168.2.50:9999 and 192.168.2.255:9999")

packets = []
start = time.time()
while time.time() - start < 10.0:
    try:
        data, addr = s.recvfrom(2048)
        text = data.decode(errors='replace').strip()
        print(f"[{addr[0]}] {text}")
        packets.append((addr[0], text))
    except socket.timeout:
        break
    except Exception as e:
        print(f"Recv error: {e}")
        break

print(f"Total packets received: {len(packets)}")
s.close()
