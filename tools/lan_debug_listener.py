import socket
import datetime
import sys

UDP_IP = "0.0.0.0"
UDP_PORT = 9999

# ANSI Colors
CYAN = "\033[96m"
GREEN = "\033[92m"
YELLOW = "\033[93m"
RED = "\033[91m"
MAGENTA = "\033[95m"
RESET = "\033[0m"

def main():
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.bind((UDP_IP, UDP_PORT))
    
    print(f"{CYAN}==================================================")
    print(f"  ATOMS OS — LIVE LAN REAL-TIME TELEMETRY ENGINE  ")
    print(f"  Bound on {UDP_IP}:{UDP_PORT} | Listening Target IP 192.168.2.100")
    print(f"=================================================={RESET}\n")
    
    try:
        while True:
            data, addr = sock.recvfrom(2048)
            now = datetime.datetime.now().strftime("%H:%M:%S.%f")[:-3]
            text = data.decode('ascii', errors='replace').strip()
            
            color = GREEN
            if "PANIC" in text or "FAIL" in text or "ERROR" in text:
                color = RED
            elif "WARN" in text or "WAIT" in text:
                color = YELLOW
            elif "USB" in text or "INPUT" in text:
                color = MAGENTA
                
            print(f"{CYAN}[{now}] [{addr[0]}]{RESET} {color}{text}{RESET}", flush=True)
    except KeyboardInterrupt:
        print(f"\n{RED}[SHUTDOWN] Live Telemetry Listener Stopped.{RESET}")
        sock.close()

if __name__ == '__main__':
    main()
