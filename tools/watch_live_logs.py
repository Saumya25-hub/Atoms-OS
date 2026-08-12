import socket
import datetime
import os
import sys

UDP_IP = "0.0.0.0"
UDP_PORT = 9999
LOG_FILE = os.path.join("build", "atoms_live_kernel.log")

# ANSI Color Definitions for Windows / Terminal
CYAN = "\033[96m"
GREEN = "\033[92m"
YELLOW = "\033[93m"
RED = "\033[91m"
MAGENTA = "\033[95m"
WHITE = "\033[97m"
BOLD = "\033[1m"
RESET = "\033[0m"

os.system('') # Enable ANSI colors on Windows CMD / PowerShell

def main():
    if not os.path.exists("build"):
        os.makedirs("build")
        
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    
    try:
        sock.bind((UDP_IP, UDP_PORT))
    except Exception as e:
        print(f"{RED}[ERROR] Port {UDP_PORT} busy or in use: {e}{RESET}")
        print(f"{YELLOW}[TIP] Close any existing listener and try again!{RESET}")
        return

    print(f"{CYAN}{BOLD}================================================================={RESET}")
    print(f"{CYAN}{BOLD}  ATOMS OS — LIVE REAL-TIME BARE-METAL KERNEL LOG VIEWER         {RESET}")
    print(f"{CYAN}{BOLD}  Listening on {UDP_IP}:{UDP_PORT} | Saving to: {LOG_FILE}         {RESET}")
    print(f"{CYAN}{BOLD}================================================================={RESET}\n")

    with open(LOG_FILE, "a", encoding="utf-8") as f_out:
        f_out.write(f"\n--- ATOMS OS LIVE LOG SESSION STARTED AT {datetime.datetime.now()} ---\n")
        f_out.flush()
        
        try:
            while True:
                data, addr = sock.recvfrom(2048)
                now = datetime.datetime.now().strftime("%H:%M:%S.%f")[:-3]
                raw_str = data.decode('ascii', errors='replace').strip()
                
                # Highlight by Category
                color = GREEN
                tag = "INFO"
                if any(k in raw_str for k in ["PANIC", "FAIL", "ERROR", "ASSERT"]):
                    color = RED
                    tag = "CRITICAL"
                elif any(k in raw_str for k in ["WARN", "WAIT", "RETRY"]):
                    color = YELLOW
                    tag = "WARNING"
                elif any(k in raw_str for k in ["USB", "INPUT", "MOUSE", "KEYBOARD"]):
                    color = MAGENTA
                    tag = "HARDWARE"
                elif "HEARTBEAT" in raw_str:
                    color = CYAN
                    tag = "ALIVE"
                    
                formatted_msg = f"[{now}] [{addr[0]}] [{tag}] {raw_str}"
                
                # Print to screen in color
                print(f"{CYAN}[{now}]{RESET} {color}{BOLD}{raw_str}{RESET}", flush=True)
                
                # Save to disk
                f_out.write(formatted_msg + "\n")
                f_out.flush()
                
        except KeyboardInterrupt:
            print(f"\n{YELLOW}[LOG VIEWER] Stopped by user.{RESET}")
            sock.close()

if __name__ == '__main__':
    main()
