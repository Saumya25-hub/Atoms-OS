import socket
import json
import time
import subprocess

QEMU_EXE = r"D:\OS-QEMU-EMU\qemu\qemu-system-x86_64.exe"
proc = subprocess.Popen([QEMU_EXE, "-qmp", "tcp:127.0.0.1:4455,server,nowait", "-device", "qemu-xhci", "-device", "usb-kbd", "-trace", "usb_kbd*", "-display", "none"])
time.sleep(1)

s = socket.socket()
s.connect(('127.0.0.1', 4455))
data = s.recv(4096)
print("Greeting:", data.decode())

s.sendall(b'{"execute": "qmp_capabilities"}\r\n')
data = s.recv(4096)
print("Capabilities resp:", data.decode())

s.sendall(b'{"execute": "send-key", "arguments": {"keys": [{"type": "qcode", "data": "caps_lock"}]}}\r\n')
time.sleep(0.5)
data = s.recv(4096)
print("caps_lock resp:", data.decode())

s.sendall(b'{"execute": "send-key", "arguments": {"keys": [{"type": "qcode", "data": "num_lock"}]}}\r\n')
time.sleep(0.5)
data = s.recv(4096)
print("num_lock resp:", data.decode())

# Test send-key
s.sendall(b'{"execute": "send-key", "arguments": {"keys": [{"type": "qcode", "data": "caps_lock"}]}}\r\n')
time.sleep(0.5)
data = s.recv(4096)
print("send-key response:", data.decode())

s.close()
proc.kill()
