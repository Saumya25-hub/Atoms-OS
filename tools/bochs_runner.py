import subprocess
import os
import sys

BOCHS_PATH = r"C:\Program Files\Bochs-3.0\bochs.exe"
BOCHS_CONFIG = r"d:\Signatures_OS\bochsrc.bxrc"
LOG_PATH = r"d:\Signatures_OS\bochs_serial.log"

def run_bochs():
    print(f"[BOCHS ENGINE] Launching Bochs 3.0 with xHCI USB Telemetry...")
    if not os.path.exists(BOCHS_PATH):
        print(f"[ERROR] Bochs binary not found at {BOCHS_PATH}")
        return
    
    cmd = [BOCHS_PATH, "-q", "-f", BOCHS_CONFIG]
    try:
        proc = subprocess.Popen(cmd)
        print(f"[BOCHS ENGINE] Process started PID={proc.pid}")
    except Exception as e:
        print(f"[ERROR] Failed to start Bochs: {e}")

def parse_log():
    if os.path.exists(LOG_PATH):
        print("\n=== BOCHS SERIAL TELEMETRY LOG ===")
        with open(LOG_PATH, "r", encoding="utf-8", errors="ignore") as f:
            print(f.read())
    else:
        print("[BOCHS ENGINE] No serial log generated yet.")

if __name__ == "__main__":
    if len(sys.argv) > 1 and sys.argv[1] == "--log":
        parse_log()
    else:
        run_bochs()
