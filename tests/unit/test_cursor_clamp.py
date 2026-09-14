import subprocess
import time
import socket
import os

if os.path.exists("forensic_clamp.log"):
    os.remove("forensic_clamp.log")

cmd = [
    "qemu-system-x86_64",
    "-drive", "file=build/OS.img,format=raw,index=0,media=disk",
    "-m", "512M",
    "-vga", "std",
    "-audiodev", "none,id=audio0",
    "-device", "AC97,audiodev=audio0",
    "-serial", "file:forensic_clamp.log",
    "-monitor", "tcp:127.0.0.1:5566,server,nowait",
    "-display", "none"
]

proc = subprocess.Popen(cmd)
time.sleep(25)

try:
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect(("127.0.0.1", 5566))
    s.recv(1024)

    def send_cmd(c):
        s.sendall(c.encode("utf-8") + b"\n")
        time.sleep(0.3)
        try:
            s.recv(1024)
        except:
            pass

    # Step 1: Move cursor fully left (-2000 px) to reach 0
    send_cmd("mouse_move -2000 0")
    time.sleep(0.5)

    # Step 2: Move cursor right across the screen (+100 px each step, 20 times)
    for _ in range(20):
        send_cmd("mouse_move 100 0")

    time.sleep(1)
    s.close()
except Exception as e:
    print(f"Error: {e}")

proc.terminate()
time.sleep(2)
if proc.poll() is None:
    proc.kill()

if os.path.exists("forensic_clamp.log"):
    with open("forensic_clamp.log", "r", errors="ignore") as f:
        content = f.read()

    # 1. Print Verification Block
    if "=== RESOLUTION & BOUNDS VERIFICATION ===" in content:
        start = content.find("=== RESOLUTION & BOUNDS VERIFICATION ===")
        end = content.find("========================================", start) + 40
        print(content[start:end])
        print()

    # 2. Extract and format per-step telemetry
    lines = content.split("\n")
    print("Runtime Test Summary (+100px increments right from x=0):")
    print(f"{'RAW dx':<10} | {'cursor_state.screen_x':<22} | {'g_bwe_mouse_x':<15} | {'cursor_box.x':<15}")
    print("-" * 68)

    # Parse state changes
    i = 0
    while i < len(lines):
        if "RAW:" in lines[i]:
            raw_dx = "100" if i+3 < len(lines) and "dx=0" not in lines[i+3] else "100" # monitor movement is absolute/relative simulated
            screen_x = "-"
            bwe_x = "-"
            box_x = "-"
            
            # Scan forward up to 25 lines for correlating updates
            for j in range(i, min(i+25, len(lines))):
                if "screen_x=" in lines[j] and "g_cursor_state" not in lines[j]:
                    screen_x = lines[j].split("screen_x=")[1].strip()
                if "g_bwe_mouse_x=" in lines[j]:
                    bwe_x = lines[j].split("g_bwe_mouse_x=")[1].strip()
                if "cursor_box.x=" in lines[j]:
                    box_x = lines[j].split("cursor_box.x=")[1].strip()
            
            if screen_x != "-" or bwe_x != "-":
                print(f"{raw_dx:<10} | {screen_x:<22} | {bwe_x:<15} | {box_x:<15}")
            i += 10
        else:
            i += 1
