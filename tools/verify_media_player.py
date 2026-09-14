import os
import sys
import time
import socket
import json
import subprocess
from PIL import Image

LOG_FILE = r"build\qemu_media_player.log"
PPM_FILE_MAIN = r"build\media_player_screen.ppm"
ARTIFACT_DIR = r"C:\Users\Saumya Chaudhari\.gemini\antigravity-ide\brain\088d2ad9-783a-4fcb-a887-541153ebcc23"
PNG_MAIN = os.path.join(ARTIFACT_DIR, "media_player_desktop.png")
PNG_DETAIL = os.path.join(ARTIFACT_DIR, "media_player_window_detail.png")

for p in [LOG_FILE, PPM_FILE_MAIN]:
    if os.path.exists(p):
        try: os.remove(p)
        except: pass

print("[MEDIA-RUNNER] Launching QEMU in pure UEFI mode with Intel HDA...")
proc = subprocess.Popen([
    r"D:\OS-QEMU-EMU\qemu\qemu-system-x86_64.exe",
    "-drive", r"if=pflash,format=raw,readonly=on,file=D:\OS-QEMU-EMU\qemu\share\edk2-x86_64-code.fd",
    "-drive", r"file=build\atoms_uefi_test.img,format=raw",
    "-device", "qemu-xhci",
    "-device", "usb-kbd",
    "-device", "usb-tablet",
    "-device", "intel-hda",
    "-device", "hda-duplex",
    "-netdev", "user,id=net0",
    "-device", "e1000,netdev=net0",
    "-m", "2048M",
    "-qmp", "tcp:127.0.0.1:4465,server,nowait",
    "-serial", f"file:{LOG_FILE}",
    "-display", "none"
])

print("[MEDIA-RUNNER] Waiting for Desktop or Login Supervisor Loop in serial log...")
reached_login = False
direct_desktop = False
for i in range(45):
    time.sleep(1)
    if os.path.exists(LOG_FILE):
        with open(LOG_FILE, "r", errors="ignore") as f:
            content = f.read()
            if "Entering Interactive Login Supervisor Loop" in content:
                print(f"[MEDIA-RUNNER] Reached login loop in {i+1}s!")
                reached_login = True
                break
            elif "DESKTOP_VISIBLE" in content or "Kernel Debug Shell" in content:
                print(f"[MEDIA-RUNNER] Reached direct desktop in {i+1}s!")
                direct_desktop = True
                break

if not reached_login and not direct_desktop:
    print("[MEDIA-RUNNER] ERROR: Did not reach desktop or login loop within timeout!")
    proc.kill()
    sys.exit(1)

time.sleep(1.0)
s = socket.socket()
s.connect(('127.0.0.1', 4465))
s.recv(1024)
s.sendall(b'{"execute": "qmp_capabilities"}\r\n')
s.recv(1024)

def send_qcode(k, hold=150):
    msg = json.dumps({
        "execute": "send-key",
        "arguments": {
            "keys": [{"type": "qcode", "data": k}],
            "hold-time": hold
        }
    }) + "\r\n"
    s.sendall(msg.encode())
    s.recv(1024)
    time.sleep(0.35)

if reached_login and not direct_desktop:
    # 1. Dismiss lock screen
    print("[MEDIA-RUNNER] Dismissing lock screen...")
    send_qcode("spc")
    time.sleep(3.5)

    # 2. Type password 'admin123'
    print("[MEDIA-RUNNER] Typing password 'admin123'...")
    for key in ["a", "d", "m", "i", "n", "1", "2", "3"]:
        send_qcode(key)

    time.sleep(1.0)
    print("[MEDIA-RUNNER] Pressing Enter...")
    send_qcode("ret")
    time.sleep(0.5)
    send_qcode("ret")

# 3. Wait for Media Player launch and playback
print("[MEDIA-RUNNER] Monitoring for Media Player initialization & bitstream decode...")
player_active = False
frame_decoded_count = 0
for i in range(40):
    time.sleep(1.0)
    if os.path.exists(LOG_FILE):
        with open(LOG_FILE, "r", errors="ignore") as f:
            content = f.read()
        if "BOS MEDIA PLAYER" in content and not player_active:
            print("[MEDIA-RUNNER] BOS Media Player Initialized!")
            player_active = True
        if "[FRAME_DEBUG]" in content or "[VIDEO] frame" in content or "DECODE RESULT: PASS" in content:
            frame_decoded_count = content.count("[VIDEO] frame") + content.count("[FRAME_DEBUG]")
            print(f"[MEDIA-RUNNER] Decoded frames detected (#{frame_decoded_count})...")
            if frame_decoded_count >= 2:
                print(f"[MEDIA-RUNNER] Active Real Playback Verified ({frame_decoded_count} decoded frame telemetry events)!")
                break

# Wait 2 seconds for continuous playback rendering
time.sleep(2.0)

# 4. Screendump Media Player window
print("[MEDIA-RUNNER] Capturing QMP screendump of Media Player playback...")
s.sendall(json.dumps({
    "execute": "screendump",
    "arguments": {"filename": PPM_FILE_MAIN.replace("\\", "/")}
}).encode() + b"\r\n")
s.recv(1024)
time.sleep(2.0)

s.close()
proc.kill()
try: proc.wait(timeout=5)
except: pass
print("[MEDIA-RUNNER] QEMU terminated cleanly.")

# 5. Process Screenshots
if os.path.exists(PPM_FILE_MAIN):
    try:
        img = Image.open(PPM_FILE_MAIN)
        img.save(PNG_MAIN)
        print(f"[MEDIA-RUNNER] Saved full desktop capture ({img.width}x{img.height}) -> {PNG_MAIN}")

        # Crop Media Player window (x=40, y=40, w=960, h=580)
        box = (30, 30, 40 + 960 + 20, 40 + 580 + 20)
        win_crop = img.crop(box)
        win_crop.save(PNG_DETAIL)
        print(f"[MEDIA-RUNNER] Saved window detail capture -> {PNG_DETAIL}")
    except Exception as ex:
        print(f"[MEDIA-RUNNER] Screenshot conversion error: {ex}")
else:
    print(f"[MEDIA-RUNNER] Warning: {PPM_FILE_MAIN} not found.")

# 6. Audit Serial Log
print("\n=======================================================")
print(" >>> FIRST NATIVE MEDIA PLAYER AUDIT <<< ")
print("=======================================================")
if os.path.exists(LOG_FILE):
    with open(LOG_FILE, "r", errors="ignore") as f:
        log_content = f.read()

    checks = {
        "VFS ESP Mount (/)": "[VFS] Mount Manager Initialized" in log_content or "VFS OPEN: PASS" in log_content,
        "MP4 Bitstream Demux": "[MEDIA_DEBUG] MP4 DEMUX: PASS" in log_content or "[MP4] moov parsed" in log_content,
        "H.264 Codec Detection": "[MEDIA_DEBUG] CODEC: H264" in log_content and "[MEDIA_DEBUG] H264 INIT: PASS" in log_content,
        "720p HD Resolution": "1280x720" in log_content,
        "Universal Audio Pipeline (Intel HDA)": "Stereo" in log_content and ("48000Hz" in log_content or "44100 Hz" in log_content),
        "Universal Video HAL Capability Probe": "[GPU] probe:" in log_content or "Universal Video Acceleration HAL" in log_content,
        "Smart CPU Fallback Bridge Active": "SMART CPU SOFTWARE FALLBACK BRIDGE ENGAGED" in log_content or "backend selected = SOFTWARE" in log_content,
        "Real Frame Decode (FFmpeg CABAC / CRC32)": "[MEDIA_DEBUG] DECODE RESULT: PASS" in log_content and "decoded_crc=" in log_content,
        "BOSurface Frame Presentation": "[BOSURFACE] frame presented" in log_content,
        "Canvas Viewport Render": "Canvas on_paint_video: Triggered" in log_content or "Present Success: TRUE" in log_content,
        "Zero Kernel Panics": "Kernel Panic" not in log_content and "CRITICAL PANIC" not in log_content
    }

    all_passed = True
    for name, passed in checks.items():
        status = "PASS" if passed else "FAIL"
        print(f"  [{status}] {name}")
        if not passed:
            all_passed = False

    print("=======================================================")
    if all_passed:
        print(" >>> FINAL VERDICT: PASS (REAL PLAYBACK END-TO-END VERIFIED) <<< ")
    else:
        print(" >>> FINAL VERDICT: INVESTIGATE FAILURES <<< ")
    print("=======================================================\n")
    sys.exit(0 if all_passed else 1)
else:
    print("[MEDIA-RUNNER] ERROR: Serial log does not exist!")
    sys.exit(1)
