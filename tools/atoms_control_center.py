import socket
import threading
import datetime
import subprocess
import os
import sys
import ctypes
import tkinter as tk
from tkinter import scrolledtext, messagebox

TARGET_MAC = "A0:AD:9F:C5:81:27"
TARGET_IP  = "192.168.2.100"
SERVER_IP  = "192.168.2.1"
UDP_IP     = "0.0.0.0"
UDP_PORT   = 9999
LOG_FILE   = os.path.join("build", "atoms_live_kernel.log")

def is_admin():
    try:
        return ctypes.windll.shell32.IsUserAnAdmin()
    except:
        return False

def add_firewall_rule():
    """Add Windows Firewall inbound rule for UDP 9999 silently."""
    try:
        subprocess.run([
            "netsh", "advfirewall", "firewall", "add", "rule",
            "name=Atoms OS UDP 9999",
            "dir=in", "action=allow", "protocol=UDP", "localport=9999"
        ], capture_output=True, timeout=5)
    except Exception:
        pass

class AtomsControlCenterApp:
    def __init__(self, root):
        self.root = root
        self.root.title("ATOMS OS — PXE & LIVE TELEMETRY CONTROL CENTER")
        self.root.geometry("960x680")
        self.root.configure(bg="#1e1e2e")

        self.running = True
        self.sock = None
        self.packets_received = 0

        self.build_ui()

        # Add firewall rule silently
        threading.Thread(target=add_firewall_rule, daemon=True).start()

        # Start listener
        self.listener_thread = threading.Thread(target=self.udp_listener_loop, daemon=True)
        self.listener_thread.start()

        self.root.protocol("WM_DELETE_WINDOW", self.on_close)

    def build_ui(self):
        # Header
        hdr = tk.Frame(self.root, bg="#181825", pady=8, padx=12)
        hdr.pack(fill="x")
        tk.Label(hdr, text="⚛️  ATOMS OS — PXE & LIVE TELEMETRY COMMAND CENTER",
                 font=("Segoe UI", 13, "bold"), bg="#181825", fg="#89dceb").pack(side="left")
        tk.Label(hdr, text=f"Server: {SERVER_IP}  |  Target: {TARGET_IP}  [{TARGET_MAC}]",
                 font=("Segoe UI", 9), bg="#181825", fg="#a6adc8").pack(side="right")

        # Toolbar
        tb = tk.Frame(self.root, bg="#1e1e2e", pady=6, padx=10)
        tb.pack(fill="x")
        self._btn(tb, "⚡ Wake-On-LAN (Power ON PC)", "#a6e3a1", self.send_wol).pack(side="left", padx=4)
        self._btn(tb, "🚀 Restart PXE Server",        "#89b4fa", self.restart_pxe).pack(side="left", padx=4)
        self._btn(tb, "🧹 Clear Logs",                "#313244", self.clear_logs, fg="#cdd6f4").pack(side="left", padx=4)
        self._btn(tb, "❌ Exit", "#f38ba8", self.on_close).pack(side="right", padx=4)

        # Packet count label
        self.pkt_label = tk.Label(tb, text="Packets: 0", font=("Segoe UI", 9, "bold"),
                                   bg="#1e1e2e", fg="#a6e3a1")
        self.pkt_label.pack(side="right", padx=10)

        # Log terminal
        lf = tk.Frame(self.root, bg="#1e1e2e", padx=10, pady=4)
        lf.pack(fill="both", expand=True)
        tk.Label(lf, text=f"▶  LIVE KERNEL TELEMETRY STREAM (UDP Port {UDP_PORT}):",
                 font=("Segoe UI", 10, "bold"), bg="#1e1e2e", fg="#cdd6f4").pack(anchor="w", pady=(0,3))

        self.text = scrolledtext.ScrolledText(lf, wrap="word", bg="#11111b", fg="#cdd6f4",
                                              font=("Consolas", 10), relief="flat", bd=2,
                                              insertbackground="#cdd6f4")
        self.text.pack(fill="both", expand=True)

        self.text.tag_config("TIME",     foreground="#89dceb")
        self.text.tag_config("INFO",     foreground="#a6e3a1")
        self.text.tag_config("WARN",     foreground="#f9e2af")
        self.text.tag_config("CRITICAL", foreground="#f38ba8", font=("Consolas", 10, "bold"))
        self.text.tag_config("HW",       foreground="#cba6f7")
        self.text.tag_config("ALIVE",    foreground="#89dceb")
        self.text.tag_config("SYS",      foreground="#a6adc8", font=("Consolas", 9, "italic"))

        # Status bar
        self.status = tk.Label(self.root, text="STATUS: Starting UDP listener on port 9999...",
                               font=("Segoe UI", 9), bg="#181825", fg="#f9e2af",
                               anchor="w", padx=10, pady=3)
        self.status.pack(fill="x", side="bottom")

    def _btn(self, parent, text, bg, cmd, fg="#11111b"):
        return tk.Button(parent, text=text, font=("Segoe UI", 10, "bold"),
                         bg=bg, fg=fg, activebackground=bg, relief="flat",
                         padx=10, pady=4, command=cmd)

    def log(self, tag, msg):
        if not self.running:
            return
        now = datetime.datetime.now().strftime("%H:%M:%S.%f")[:-3]
        self.text.insert("end", f"[{now}] ", "TIME")
        self.text.insert("end", f"{msg}\n", tag)
        self.text.see("end")
        try:
            os.makedirs("build", exist_ok=True)
            with open(LOG_FILE, "a", encoding="utf-8") as f:
                f.write(f"[{now}] [{tag}] {msg}\n")
        except Exception:
            pass

    def udp_listener_loop(self):
        try:
            self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            self.sock.settimeout(1.0)
            self.sock.bind((UDP_IP, UDP_PORT))
            self.root.after(0, self.status.config,
                            {"text": f"STATUS: LISTENING ON {UDP_IP}:{UDP_PORT} | Waiting for Atoms OS...",
                             "fg": "#a6e3a1"})
            self.root.after(0, self.log, "SYS",
                            f"UDP socket bound on {UDP_IP}:{UDP_PORT} — waiting for bare-metal packets...")
        except Exception as e:
            self.root.after(0, self.status.config,
                            {"text": f"ERROR: Cannot bind port {UDP_PORT}: {e}", "fg": "#f38ba8"})
            self.root.after(0, self.log, "CRITICAL",
                            f"Socket bind FAILED on port {UDP_PORT}: {e}  →  Run as Administrator!")
            return

        while self.running:
            try:
                data, addr = self.sock.recvfrom(2048)
                raw = data.decode("ascii", errors="replace").strip()
                if not raw:
                    continue

                self.packets_received += 1
                tag = "INFO"
                if any(k in raw for k in ["PANIC", "FAIL", "ERROR", "ASSERT"]):
                    tag = "CRITICAL"
                elif any(k in raw for k in ["WARN", "WAIT", "RETRY"]):
                    tag = "WARN"
                elif any(k in raw for k in ["USB", "INPUT", "MOUSE", "KBD"]):
                    tag = "HW"
                elif "HEARTBEAT" in raw:
                    tag = "ALIVE"

                self.root.after(0, self.log, tag, f"[{addr[0]}] {raw}")
                self.root.after(0, self.pkt_label.config,
                                {"text": f"Packets: {self.packets_received}"})
                self.root.after(0, self.status.config,
                                {"text": f"STATUS: LIVE ✅ | {self.packets_received} packets from {addr[0]}",
                                 "fg": "#a6e3a1"})
            except socket.timeout:
                continue
            except Exception:
                break

    def send_wol(self):
        try:
            mac_bytes = bytes.fromhex(TARGET_MAC.replace(":", ""))
            pkt = b"\xff" * 6 + mac_bytes * 16
            with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as s:
                s.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
                s.sendto(pkt, ("192.168.2.255", 9))
                s.sendto(pkt, ("255.255.255.255", 9))
            self.log("HW", f"⚡ Wake-On-LAN sent to {TARGET_MAC}")
            self.status.config(text=f"STATUS: WOL sent to {TARGET_MAC}", fg="#f9e2af")
        except Exception as e:
            messagebox.showerror("WOL Error", str(e))

    def restart_pxe(self):
        try:
            subprocess.Popen([sys.executable, os.path.join("tools", "pxe_server.py")],
                             creationflags=subprocess.CREATE_NEW_CONSOLE)
            self.log("INFO", "🚀 PXE Server launched in new window.")
            self.status.config(text="STATUS: PXE Server started", fg="#89b4fa")
        except Exception as e:
            messagebox.showerror("PXE Error", str(e))

    def clear_logs(self):
        self.text.delete("1.0", "end")
        self.log("SYS", "Log cleared.")

    def on_close(self):
        self.running = False
        if self.sock:
            try:
                self.sock.close()
            except Exception:
                pass
        self.root.destroy()
        sys.exit(0)

if __name__ == "__main__":
    if not is_admin():
        # Re-run as admin automatically
        ctypes.windll.shell32.ShellExecuteW(
            None, "runas", sys.executable,
            f'"{os.path.abspath(__file__)}"', os.getcwd(), 1)
        sys.exit(0)
    root = tk.Tk()
    app = AtomsControlCenterApp(root)
    root.mainloop()
