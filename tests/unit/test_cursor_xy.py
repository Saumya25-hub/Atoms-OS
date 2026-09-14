import subprocess
import time
import socket
import os

if os.path.exists("forensic_xy.log"):
    os.remove("forensic_xy.log")

cmd = [
    "qemu-system-x86_64",
    "-drive", "file=build/OS.img,format=raw,index=0,media=disk",
    "-m", "512M",
    "-vga", "std",
    "-audiodev", "none,id=audio0",
    "-device", "AC97,audiodev=audio0",
    "-serial", "file:forensic_xy.log",
    "-monitor", "tcp:127.0.0.1:5566,server,nowait",
    "-display", "none"
]

print("Launching QEMU...")
proc = subprocess.Popen(cmd)

print("Waiting 25 seconds for OS boot...")
time.sleep(25)

print("Connecting to QEMU monitor...")
try:
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect(("127.0.0.1", 5566))
    s.recv(1024) # Banner

    def send_cmd(c):
        print(f"Monitor command: {c}")
        s.sendall(c.encode("utf-8") + b"\n")
        time.sleep(0.5)
        try:
            print(s.recv(1024).decode("utf-8", errors="ignore"))
        except:
            pass

    # Send horizontal movement
    send_cmd("mouse_move 100 0")
    time.sleep(1)
    send_cmd("mouse_move -50 0")
    time.sleep(1)
    send_cmd("mouse_move 200 0")
    time.sleep(1)

    # Send vertical movement
    send_cmd("mouse_move 0 100")
    time.sleep(1)

    # Send diagonal movement
    send_cmd("mouse_move 50 50")
    time.sleep(2)
    s.close()
except Exception as e:
    print(f"Monitor connection error: {e}")

print("Killing QEMU...")
proc.terminate()
time.sleep(2)
if proc.poll() is None:
    proc.kill()

print("Checking forensic_xy.log...")
if os.path.exists("forensic_xy.log"):
    with open("forensic_xy.log", "r", errors="ignore") as f:
        lines = f.readlines()
    print(f"Total lines captured: {len(lines)}")
    # Print lines matching instrumentation points
    keywords = ["RAW:", "CURSOR STATE", "BWE INPUT", "HOTSPOT", "DRAW"]
    for i, line in enumerate(lines):
        for kw in keywords:
            if kw in line:
                # print the next 6 lines
                block = "".join(lines[i:min(i+7, len(lines))])
                print("----------------------------------------")
                print(block.strip())
                break
else:
    print("forensic_xy.log not found!")
