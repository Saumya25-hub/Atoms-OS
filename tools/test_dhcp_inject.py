import socket

# Send a mock DHCPDISCOVER to 127.0.0.1:67 and 192.168.2.1:67
# Construct minimal valid DHCP packet
import struct

data = bytearray(300)
data[0] = 1 # BOOTREQUEST
data[1] = 1 # Ethernet
data[2] = 6 # hlen
# xid
data[4:8] = b'\x12\x34\x56\x78'
# MAC
data[28:34] = bytes.fromhex("A0AD9FC58127")
# Magic cookie
data[236:240] = b'\x63\x82\x53\x63'
# Option 53: DHCPDISCOVER (len 1, val 1)
data[240:243] = b'\x35\x01\x01'
# End
data[243] = 255

s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
s.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)

print("Sending mock DHCPDISCOVER to 192.168.2.1:67...")
try:
    s.sendto(data, ("192.168.2.1", 67))
    print("[SUCCESS] Sent to 192.168.2.1:67")
except Exception as e:
    print(f"[FAIL] 192.168.2.1:67: {e}")

print("Sending mock DHCPDISCOVER to 127.0.0.1:67...")
try:
    s.sendto(data, ("127.0.0.1", 67))
    print("[SUCCESS] Sent to 127.0.0.1:67")
except Exception as e:
    print(f"[FAIL] 127.0.0.1:67: {e}")

s.close()
