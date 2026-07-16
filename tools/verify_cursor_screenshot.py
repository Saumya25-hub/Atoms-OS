import subprocess
import time
import socket
import os
from PIL import Image

def main():
    log_file = "cursor_verification.log"
    ppm_file = "cursor_desktop.ppm"
    out_png = "C:/Users/Saumya Chaudhari/.gemini/antigravity-ide/brain/45fcddda-078e-40cf-b2e0-97e44087fee6/cursor_desktop_restored.png"
    
    if os.path.exists(log_file):
        os.remove(log_file)
    if os.path.exists(ppm_file):
        os.remove(ppm_file)
        
    print("Launching QEMU for cursor verification...")
    cmd = [
        "qemu-system-x86_64",
        "-drive", "file=build/OS.img,format=raw,index=0,media=disk",
        "-m", "512M",
        "-vga", "std",
        "-audiodev", "none,id=audio0",
        "-device", "AC97,audiodev=audio0",
        "-serial", f"file:{log_file}",
        "-monitor", "tcp:127.0.0.1:5566,server,nowait",
        "-display", "none"
    ]
    proc = subprocess.Popen(cmd)
    
    # Wait for boot to reach main loop
    print("Waiting for ATOMS OS boot and shell startup...")
    booted = False
    for _ in range(30):
        time.sleep(1)
        if os.path.exists(log_file):
            with open(log_file, "r", errors="ignore") as f:
                content = f.read()
                if "Entering main loop" in content or "SwapBuffers" in content:
                    booted = True
                    break
    
    if not booted:
        print("Warning: Boot loop threshold reached, proceeding with monitor connection...")
    else:
        print("ATOMS OS successfully booted into main loop! Waiting 2s for UI stabilization...")
        time.sleep(2)
        
    # Connect to QEMU monitor
    print("Connecting to QEMU TCP monitor on port 5566...")
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.settimeout(5)
        s.connect(("127.0.0.1", 5566))
        time.sleep(0.5)
        greeting = s.recv(1024)
        print("Monitor greeting:", greeting.decode("ascii", errors="ignore").strip())
        
        # Inject mouse movement to trigger UpdatePosition & OnCompositorRedraw
        print("Sending mouse movements...")
        s.sendall(b"mouse_move 100 100\r\n")
        time.sleep(1)
        s.sendall(b"mouse_move -50 -50\r\n")
        time.sleep(1)
        s.sendall(b"mouse_move 20 20\r\n")
        time.sleep(1.5)
        
        print("Capturing screendump...")
        s.sendall(f"screendump {ppm_file}\r\n".encode("ascii"))
        time.sleep(2)
        
        s.sendall(b"quit\r\n")
        s.close()
    except Exception as e:
        print(f"Monitor connection error: {e}")
        
    time.sleep(1)
    if proc.poll() is None:
        proc.terminate()
        proc.wait(timeout=5)
        
    # Check log for fallback diagnostics
    if os.path.exists(log_file):
        with open(log_file, "r", errors="ignore") as f:
            lines = f.readlines()
        print("\n--- Serial Telemetry Excerpts ---")
        for line in lines[-20:]:
            print("  ", line.strip())
            
    # Process PPM screenshot
    if os.path.exists(ppm_file):
        print(f"\nProcessing screenshot {ppm_file}...")
        img = Image.open(ppm_file)
        print(f"Image dimensions: {img.width}x{img.height}, mode: {img.mode}")
        img.save(out_png)
        print(f"Saved artifact PNG: {out_png}")
        print(f"Screenshot successfully captured and converted to {out_png}")
    else:
        print("Error: Screendump PPM file not found.")

if __name__ == "__main__":
    main()
