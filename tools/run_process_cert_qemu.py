import os
import sys
import time
import socket
import subprocess
from PIL import Image

QEMU_EXE = r"D:\OS-QEMU-EMU\qemu\qemu-system-x86_64.exe"
UEFI_BIOS = r"D:\OS-QEMU-EMU\qemu\share\edk2-x86_64-code.fd"
IMAGE_PATH = r"d:\Signatures_OS\build\atoms_uefi_test.img"
SERIAL_LOG = r"d:\Signatures_OS\build\process_cert_serial.log"
PPM_PATH = r"d:\Signatures_OS\build\process_cert_screen.ppm"
PNG_PATH = r"d:\Signatures_OS\build\process_cert_screen.png"

def run_test():
    for p in [SERIAL_LOG, PPM_PATH, PNG_PATH]:
        if os.path.exists(p):
            try: os.remove(p)
            except: pass

    cmd = [
        QEMU_EXE,
        "-cpu", "qemu64",
        "-smp", "2",
        "-m", "2048",
        "-vga", "std",
        "-drive", f"if=pflash,format=raw,readonly=on,file={UEFI_BIOS}",
        "-device", "qemu-xhci,id=xhci0",
        "-device", "usb-mouse,bus=xhci0.0",
        "-device", "usb-kbd",
        "-drive", f"file={IMAGE_PATH},format=raw",
        "-serial", f"file:{SERIAL_LOG}",
        "-monitor", "telnet:127.0.0.1:4444,server,nowait",
        "-display", "none",
        "-no-reboot"
    ]

    print("[RUNNER] Starting QEMU with serial output and telnet monitor...")
    proc = subprocess.Popen(cmd)

    credentials_sent = False
    login_authenticated = False
    login_sent_time = 0
    cert_finished = False
    verdict_pass = False
    start_time = time.time()
    last_log_pos = 0
    telnet_sock = None

    try:
        while time.time() - start_time < 180:
            if proc.poll() is not None:
                print(f"[RUNNER] QEMU exited early with code {proc.returncode}")
                break

            if os.path.exists(SERIAL_LOG):
                try:
                    with open(SERIAL_LOG, "r", encoding="utf-8", errors="replace") as f:
                        f.seek(last_log_pos)
                        new_content = f.read()
                        last_log_pos = f.tell()

                        if new_content:
                            for line in new_content.splitlines():
                                line_str = line.strip()
                                if any(tag in line_str for tag in [
                                    "ROOK", "LOGIN", "PROCESS_SPAWN", "L5", "ELF_BUF", "P01", "P02", "P03",
                                    "P04", "P05", "P06", "P07", "P08", "P09", "P10", "P11", "P12",
                                    "CERTIFICATION", "MOJO", "CHILD", "timestamp"
                                ]):
                                    print(f"  [QEMU LOG] {line_str}")

                                if "Entering Interactive Login Supervisor Loop" in line_str and not credentials_sent:
                                    print("[RUNNER] Login screen detected! Connecting to QEMU monitor...")
                                    time.sleep(1.5)
                                    try:
                                        telnet_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                                        telnet_sock.connect(("127.0.0.1", 4444))
                                        telnet_sock.settimeout(1.0)
                                        try: telnet_sock.recv(4096)
                                        except: pass

                                        def do_login():
                                            print("[RUNNER] Sending Space to wake up lock screen...")
                                            telnet_sock.sendall(b"sendkey spc\n")
                                            time.sleep(2.0) # Full transition to LOGIN_STATE_SIGN_IN (400ms anim + margin)
                                            print("[RUNNER] Sending 'admin123' password...")
                                            for k in ["a", "d", "m", "i", "n", "1", "2", "3"]:
                                                telnet_sock.sendall(f"sendkey {k}\n".encode("ascii"))
                                                time.sleep(0.30)
                                            # Extra delay before Enter to ensure all keys processed
                                            time.sleep(0.5)
                                            print("[RUNNER] Sending Enter (ret)...")
                                            telnet_sock.sendall(b"sendkey ret\n")
                                            time.sleep(0.5)
                                            # Send Enter a second time for reliability
                                            telnet_sock.sendall(b"sendkey ret\n")
                                            time.sleep(0.3)
                                            print("[RUNNER] Sent Enter. Awaiting desktop handoff...")

                                        do_login()
                                        credentials_sent = True
                                        login_sent_time = time.time()
                                    except Exception as ex:
                                        print(f"[RUNNER] Telnet send failed: {ex}")

                                if "DESKTOP_LAUNCH_BEGIN" in line_str or "Process Engine Subsystems Active" in line_str:
                                    login_authenticated = True

                                if "ALL 12 PROCESS/IPC CERTIFICATION TESTS: PASS" in line_str:
                                    print("\n=======================================================")
                                    print(" >>> SUCCESS: ALL 12 TESTS CERTIFIED PASS! <<< ")
                                    print("=======================================================\n")
                                    cert_finished = True
                                    verdict_pass = True
                                    break
                                elif "PROCESS/IPC CERTIFICATION VERDICT: FAIL" in line_str:
                                    print("\n=======================================================")
                                    print(" >>> FAILURE: CERTIFICATION SUITE FAILED! <<< ")
                                    print("=======================================================\n")
                                    cert_finished = True
                                    verdict_pass = False
                                    break
                except Exception as read_ex:
                    pass

            if credentials_sent and not login_authenticated and (time.time() - login_sent_time) > 10.0:
                print("[RUNNER] Authentication not detected after 10s. Retrying login sequence...")
                try:
                    # Clear any existing password field content with backspaces
                    for _ in range(20):
                        telnet_sock.sendall(b"sendkey backspace\n")
                        time.sleep(0.05)
                    time.sleep(0.3)
                    telnet_sock.sendall(b"sendkey spc\n")
                    time.sleep(2.0)
                    for k in ["a", "d", "m", "i", "n", "1", "2", "3"]:
                        telnet_sock.sendall(f"sendkey {k}\n".encode("ascii"))
                        time.sleep(0.30)
                    time.sleep(0.5)
                    telnet_sock.sendall(b"sendkey ret\n")
                    time.sleep(0.5)
                    telnet_sock.sendall(b"sendkey ret\n")
                    print("[RUNNER] Resent 'admin123' + Enter.")
                    login_sent_time = time.time()
                except Exception as ex:
                    print(f"[RUNNER] Retry failed: {ex}")

            if cert_finished:
                break
            time.sleep(0.5)

        # Take screendump if telnet available
        if telnet_sock:
            try:
                print("[RUNNER] Requesting screendump...")
                time.sleep(1.0)
                telnet_sock.sendall(b"screendump d:/Signatures_OS/build/process_cert_screen.ppm\n")
                time.sleep(1.0)
                telnet_sock.sendall(b"quit\n")
                telnet_sock.close()
            except Exception as e:
                pass
    finally:
        if proc.poll() is None:
            proc.terminate()
            try: proc.wait(timeout=3)
            except: proc.kill()

    # Convert PPM if exists
    if os.path.exists(PPM_PATH):
        try:
            img = Image.open(PPM_PATH)
            img.save(PNG_PATH, "PNG")
            print(f"[RUNNER] Saved screenshot to {PNG_PATH}")
        except Exception as e:
            print(f"[RUNNER] Image conversion failed: {e}")

    return verdict_pass

if __name__ == "__main__":
    success = run_test()
    sys.exit(0 if success else 1)
