import socket
import sys

print("Testing UDP Socket Binding on Windows...")

# Test 1: Bind 0.0.0.0:67
try:
    s1 = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s1.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    s1.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
    s1.bind(("0.0.0.0", 67))
    print("[SUCCESS] S1 bound to 0.0.0.0:67")
except Exception as e:
    print(f"[FAIL] S1 0.0.0.0:67: {e}")

# Test 2: Bind 192.168.2.1:67
try:
    s2 = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s2.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    s2.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
    s2.bind(("192.168.2.1", 67))
    print("[SUCCESS] S2 bound to 192.168.2.1:67")
except Exception as e:
    print(f"[FAIL] S2 192.168.2.1:67: {e}")

# Test sending broadcast from s2
try:
    s2.sendto(b"TEST", ("255.255.255.255", 68))
    print("[SUCCESS] S2 sent broadcast to 255.255.255.255:68")
except Exception as e:
    print(f"[FAIL] S2 sendto: {e}")

# Close
s1.close()
s2.close()
print("All socket tests completed.")
