import subprocess
import time
import os
import socket
import json
from PIL import Image

def main():
    ppm_path = r"d:\Signatures_OS\build\explorer_screen.ppm"
    png_path = r"C:\Users\Saumya Chaudhari\.gemini\antigravity-ide\brain\aaf4e6d2-87b2-4ab5-982f-40b241679c16\explorer_vm_screenshot.png"

    if os.path.exists(ppm_path):
        try: os.remove(ppm_path)
        except: pass

    qemu_cmd = [
        "qemu-system-x86_64",
        "-m", "1024M",
        "-vga", "std",
        "-drive", "file=build/OS.img,format=raw,index=0,media=disk",
        "-usb", "-device", "usb-tablet",
        "-qmp", "tcp:127.0.0.1:4444,server,nowait",
        "-display", "none"
    ]

    print("Starting QEMU process with QMP...")
    p = subprocess.Popen(qemu_cmd, cwd=r"d:\Signatures_OS")
    time.sleep(6)
    
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect(("127.0.0.1", 4444))
        
        # Read greeting
        data = s.recv(1024)
        print("QMP Greeting:", data.decode('utf-8', errors='ignore'))
        
        # QMP capabilities
        s.sendall(b'{"execute": "qmp_capabilities"}\n')
        data = s.recv(1024)
        print("QMP Caps:", data.decode('utf-8', errors='ignore'))
        
        # Screendump command
        dump_cmd = {"execute": "screendump", "arguments": {"filename": "build/explorer_screen.ppm"}}
        s.sendall((json.dumps(dump_cmd) + "\n").encode('utf-8'))
        time.sleep(2)
        data = s.recv(1024)
        print("QMP Dump response:", data.decode('utf-8', errors='ignore'))
        
        # Quit QEMU
        s.sendall(b'{"execute": "quit"}\n')
        s.close()
    except Exception as e:
        print("QMP Error:", e)

    time.sleep(1)
    try: p.kill()
    except: pass

    if os.path.exists(ppm_path):
        img = Image.open(ppm_path)
        img.save(png_path)
        print(f"SUCCESS: Screenshot saved to {png_path}")
    else:
        print("ERROR: Screendump failed")

if __name__ == "__main__":
    main()
