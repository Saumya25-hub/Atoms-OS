import socket
import struct
import threading
import datetime
import subprocess
import os
import sys
import ctypes
import time
import queue
from collections import deque
import tkinter as tk
from tkinter import scrolledtext, messagebox, filedialog

# =====================================================================
# ATOMS MATRIX DEBUG ENGINE (AMDE) V1.0 — PRODUCTION TELEMETRY CONSOLE
# Target Platform: Bare-Metal Intel Haswell H81 Motherboard (ASUS B750MK)
# Host Controller: Realtek R8168/8111 PCIe Gigabit NIC
# =====================================================================

TARGET_MAC = "A0:AD:9F:C5:81:27"
TARGET_IP  = "192.168.2.100"
SERVER_IP  = "192.168.2.1"
UDP_IP     = "0.0.0.0"
UDP_PORT   = 9999
MAX_LOG_RECORDS = 100000
MAX_UI_LINES    = 5000

def is_admin():
    try:
        return ctypes.windll.shell32.IsUserAnAdmin()
    except Exception:
        return False

def add_firewall_rule():
    """Add Windows Firewall inbound rule for UDP 9999 silently."""
    try:
        subprocess.run([
            "netsh", "advfirewall", "firewall", "add", "rule",
            "name=AMDE UDP 9999",
            "dir=in", "action=allow", "protocol=UDP", "localport=9999"
        ], capture_output=True, timeout=5)
    except Exception:
        pass

class AMDE_App:
    def __init__(self, root):
        self.root = root
        self.root.title("ATOMS MATRIX DEBUG ENGINE (AMDE) V1.0")
        self.root.geometry("1100x750")
        self.root.configure(bg="#11111b")

        self.running = True
        self.sock = None
        self.last_packet_time = 0
        
        # High-performance Ring Buffer & Queues
        self.log_ring_buffer = deque(maxlen=MAX_LOG_RECORDS)
        self.ingest_queue    = queue.Queue()
        
        # Statistics Counters
        self.packets_received     = 0
        self.duplicates_collapsed = 0
        self.hex_bytes_aggregated = 0
        
        # Smart Deduplication State
        self.last_msg_text     = None
        self.last_msg_tag      = None
        self.last_msg_line_num = None
        self.repeat_count      = 1
        
        # Hex Aggregator Buffer State
        self.hex_buffer = []

        # UI State Flags
        self.auto_scroll_var = tk.BooleanVar(value=True)
        self.filter_level_var = tk.StringVar(value="ALL")
        self.search_query_var = tk.StringVar(value="")
        self.building_in_progress = False

        self.build_ui()
        
        # Start background firewall helper
        threading.Thread(target=add_firewall_rule, daemon=True).start()

        # Start UDP Telemetry Listener Thread
        self.listener_thread = threading.Thread(target=self.udp_listener_loop, daemon=True)
        self.listener_thread.start()

        # Start 30 FPS (33ms) Batch UI Rendering Loop
        self.root.after(33, self.flush_ui_batch_loop)

        # Start 1Hz Heartbeat Connection Monitor
        self.root.after(1000, self.connection_monitor_loop)

        self.root.protocol("WM_DELETE_WINDOW", self.on_close)

    def build_ui(self):
        # 1. TOP HEADER BAR
        hdr = tk.Frame(self.root, bg="#181825", pady=8, padx=12)
        hdr.pack(fill="x")
        
        title_frame = tk.Frame(hdr, bg="#181825")
        title_frame.pack(side="left")
        
        tk.Label(title_frame, text="⚛️ ATOMS MATRIX DEBUG ENGINE (AMDE) V1.0",
                 font=("Consolas", 13, "bold"), bg="#181825", fg="#89dceb").pack(side="left")
        
        self.conn_indicator = tk.Label(title_frame, text=" [OFFLINE] ",
                                       font=("Consolas", 10, "bold"), bg="#313244", fg="#f38ba8")
        self.conn_indicator.pack(side="left", padx=10)

        info_frame = tk.Frame(hdr, bg="#181825")
        info_frame.pack(side="right")
        tk.Label(info_frame, text=f"Server: {SERVER_IP}  |  Target: {TARGET_IP} [{TARGET_MAC}]",
                 font=("Consolas", 9), bg="#181825", fg="#a6adc8").pack()

        # 2. ACTION TOOLBAR
        tb = tk.Frame(self.root, bg="#1e1e2e", pady=6, padx=10)
        tb.pack(fill="x")
        
        self.btn_build      = self._btn(tb, "🔨 BUILD",          "#89b4fa", self.cmd_build)
        self.btn_build_wake = self._btn(tb, "🚀 BUILD + WAKE",   "#a6e3a1", self.cmd_build_and_wake)
        self.btn_wake       = self._btn(tb, "⚡ WAKE",           "#cba6f7", self.cmd_wake)
        self.btn_reboot     = self._btn(tb, "🔄 REBOOT",         "#f9e2af", self.cmd_reboot)
        self.btn_shutdown   = self._btn(tb, "🛑 SHUTDOWN",       "#f38ba8", self.cmd_shutdown)
        self.btn_clear      = self._btn(tb, "🧹 CLEAR LOGS",     "#313244", self.cmd_clear_logs, fg="#cdd6f4")
        self.btn_export     = self._btn(tb, "💾 EXPORT LOGS",    "#313244", self.cmd_export_logs, fg="#cdd6f4")

        self.btn_build.pack(side="left", padx=3)
        self.btn_build_wake.pack(side="left", padx=3)
        self.btn_wake.pack(side="left", padx=3)
        self.btn_reboot.pack(side="left", padx=3)
        self.btn_shutdown.pack(side="left", padx=3)
        self.btn_clear.pack(side="left", padx=3)
        self.btn_export.pack(side="left", padx=3)

        self._btn(tb, "❌ EXIT", "#f38ba8", self.on_close).pack(side="right", padx=3)

        # 3. FILTER & SEARCH CONTROL BAR
        fb = tk.Frame(self.root, bg="#181825", pady=4, padx=10)
        fb.pack(fill="x")

        tk.Label(fb, text="🔍 Search:", font=("Consolas", 9, "bold"), bg="#181825", fg="#cdd6f4").pack(side="left", padx=(0,4))
        search_entry = tk.Entry(fb, textvariable=self.search_query_var, font=("Consolas", 9),
                                bg="#11111b", fg="#cdd6f4", insertbackground="#cdd6f4", relief="flat", bd=2, width=25)
        search_entry.pack(side="left", padx=4)
        search_entry.bind("<KeyRelease>", lambda e: self.apply_filter())

        tk.Label(fb, text=" Level:", font=("Consolas", 9, "bold"), bg="#181825", fg="#cdd6f4").pack(side="left", padx=(10,4))
        level_opt = tk.OptionMenu(fb, self.filter_level_var, "ALL", "INFO", "WARN", "ERROR", "CRITICAL", command=lambda v: self.apply_filter())
        level_opt.config(font=("Consolas", 8, "bold"), bg="#313244", fg="#cdd6f4", activebackground="#45475a", relief="flat", highlightthickness=0)
        level_opt.pack(side="left", padx=4)

        chk_scroll = tk.Checkbutton(fb, text="Auto-scroll", variable=self.auto_scroll_var,
                                    font=("Consolas", 9, "bold"), bg="#181825", fg="#a6e3a1",
                                    activebackground="#181825", activeforeground="#a6e3a1", selectcolor="#11111b")
        chk_scroll.pack(side="right", padx=10)

        # 4. LOG TERMINAL AREA
        lf = tk.Frame(self.root, bg="#11111b", padx=10, pady=4)
        lf.pack(fill="both", expand=True)

        self.text = scrolledtext.ScrolledText(lf, wrap="none", bg="#11111b", fg="#cdd6f4",
                                              font=("Consolas", 9), relief="flat", bd=2,
                                              insertbackground="#cdd6f4")
        self.text.pack(fill="both", expand=True)

        # Syntax Color Tags
        self.text.tag_config("TIME",     foreground="#89dceb")
        self.text.tag_config("INFO",     foreground="#a6e3a1")
        self.text.tag_config("WARN",     foreground="#f9e2af")
        self.text.tag_config("ERROR",    foreground="#fab387", font=("Consolas", 9, "bold"))
        self.text.tag_config("CRITICAL", foreground="#f38ba8", font=("Consolas", 9, "bold"))
        self.text.tag_config("HW",       foreground="#cba6f7")
        self.text.tag_config("ALIVE",    foreground="#89dceb")
        self.text.tag_config("HEX",      foreground="#74c7ec")
        self.text.tag_config("REPEAT",   foreground="#f5e0dc", font=("Consolas", 9, "italic"))
        self.text.tag_config("SYS",      foreground="#a6adc8", font=("Consolas", 9, "italic"))

        # 5. BOTTOM METRICS & STATUS BAR
        sb = tk.Frame(self.root, bg="#181825", padx=10, pady=4)
        sb.pack(fill="x", side="bottom")

        self.lbl_stats = tk.Label(sb, text="Packets: 0 | Duplicates Suppressed: 0 | RAM Ring Buffer: 0 / 100,000",
                                  font=("Consolas", 9), bg="#181825", fg="#a6e3a1")
        self.lbl_stats.pack(side="left")

        self.lbl_status = tk.Label(sb, text="STATUS: SYSTEM READY",
                                   font=("Consolas", 9, "bold"), bg="#181825", fg="#89dceb")
        self.lbl_status.pack(side="right")

    def _btn(self, parent, text, bg, cmd, fg="#11111b"):
        return tk.Button(parent, text=text, font=("Consolas", 9, "bold"),
                         bg=bg, fg=fg, activebackground=bg, relief="flat",
                         padx=8, pady=3, command=cmd)

    # =====================================================================
    # SMART DEDUPLICATION & HEX STREAM AGGREGATION ENGINE
    # =====================================================================

    def process_telemetry_message(self, raw_msg):
        """Pre-processes incoming message for Hex aggregation and deduplication."""
        msg = raw_msg.strip()
        if not msg:
            return

        # 1. Hex Fragment Aggregator Detector
        # Detect single byte hex tokens (e.g. '0x', 'C0', '0x', 'A8')
        if msg == "0x" or (len(msg) <= 3 and all(c in "0123456789ABCDEFabcdef" for c in msg)):
            if msg != "0x":
                self.hex_buffer.append(msg.upper())
                self.hex_bytes_aggregated += 1
            if len(self.hex_buffer) >= 16:
                hex_str = f"[HEX DUMP {len(self.hex_buffer)}B] " + " ".join(self.hex_buffer)
                self.hex_buffer.clear()
                self._dispatch_log("HEX", hex_str)
            return

        # Flush any remaining hex buffer before normal log message
        if self.hex_buffer:
            hex_str = f"[HEX DUMP {len(self.hex_buffer)}B] " + " ".join(self.hex_buffer)
            self.hex_buffer.clear()
            self._dispatch_log("HEX", hex_str)

        # Tag Severity Classifier
        tag = "INFO"
        upper = msg.upper()
        if any(k in upper for k in ["PANIC", "FAIL", "CRITICAL", "ASSERT", "ERROR"]):
            tag = "CRITICAL" if "PANIC" in upper or "CRITICAL" in upper else "ERROR"
        elif any(k in upper for k in ["WARN", "WAIT", "RETRY", "TIMEOUT"]):
            tag = "WARN"
        elif any(k in upper for k in ["USB", "INPUT", "MOUSE", "KBD", "R8168"]):
            tag = "HW"
        elif "HEARTBEAT" in upper:
            tag = "ALIVE"

        self._dispatch_log(tag, msg)

    def _dispatch_log(self, tag, msg):
        """Smart Deduplication Gate: Collapses repeating log streams."""
        now = datetime.datetime.now().strftime("%H:%M:%S.%f")[:-3]
        record = {"timestamp": now, "tag": tag, "msg": msg}
        
        self.log_ring_buffer.append(record)
        self.packets_received += 1

        # Check if identical to previous message
        if self.last_msg_text == msg and self.last_msg_tag == tag:
            self.repeat_count += 1
            self.duplicates_collapsed += 1
            # Signal UI update for existing line repeat counter
            self.ingest_queue.put(("REPEAT_UPDATE", self.repeat_count))
        else:
            self.last_msg_text = msg
            self.last_msg_tag = tag
            self.repeat_count = 1
            self.ingest_queue.put(("NEW_LINE", record))

    # =====================================================================
    # 30 FPS (33ms) BATCH UI RENDERER
    # =====================================================================

    def flush_ui_batch_loop(self):
        """30 FPS Batch rendering loop executing on the Tkinter GUI thread."""
        if not self.running:
            return

        processed = 0
        while not self.ingest_queue.empty() and processed < 200:
            item_type, payload = self.ingest_queue.get_nowait()
            processed += 1

            if item_type == "NEW_LINE":
                tag = payload["tag"]
                now = payload["timestamp"]
                msg = payload["msg"]

                # Apply Filter
                if self._matches_filter(tag, msg):
                    self.text.insert("end", f"[{now}] ", "TIME")
                    line_start = self.text.index("end-1c linestart")
                    self.text.insert("end", f"[{tag}] {msg}\n", tag)
                    self.last_msg_line_num = line_start

            elif item_type == "REPEAT_UPDATE":
                count = payload
                if self.last_msg_line_num:
                    try:
                        line_end = f"{self.last_msg_line_num} lineend"
                        # Check if line already has repeat counter suffix
                        current_line = self.text.get(self.last_msg_line_num, line_end)
                        if " [Repeated" in current_line:
                            base_text = current_line.split(" [Repeated")[0]
                        else:
                            base_text = current_line
                        
                        self.text.delete(self.last_msg_line_num, line_end)
                        self.text.insert(self.last_msg_line_num, f"{base_text} [Repeated {count:,} times]", ("REPEAT",))
                    except Exception:
                        pass

        # Maintain UI Line Limit to prevent RAM explosion
        try:
            line_count = int(self.text.index("end-1c").split(".")[0])
            if line_count > MAX_UI_LINES:
                self.text.delete("1.0", f"{line_count - MAX_UI_LINES}.0")
        except Exception:
            pass

        # Auto-scroll if enabled
        if self.auto_scroll_var.get() and processed > 0:
            self.text.see("end")

        # Update Statistics Bar
        self.lbl_stats.config(
            text=f"Packets: {self.packets_received:,} | Duplicates Collapsed: {self.duplicates_collapsed:,} | RAM Ring Buffer: {len(self.log_ring_buffer):,} / {MAX_LOG_RECORDS:,}"
        )

        self.root.after(33, self.flush_ui_batch_loop)

    def _matches_filter(self, tag, msg):
        selected_level = self.filter_level_var.get()
        if selected_level != "ALL" and tag != selected_level:
            return False

        query = self.search_query_var.get().strip().lower()
        if query and query not in msg.lower() and query not in tag.lower():
            return False

        return True

    def apply_filter(self):
        """Re-filters the current log view from memory ring buffer."""
        self.text.delete("1.0", "end")
        self.last_msg_text = None
        self.last_msg_tag = None
        self.last_msg_line_num = None
        
        for record in self.log_ring_buffer:
            tag = record["tag"]
            now = record["timestamp"]
            msg = record["msg"]
            if self._matches_filter(tag, msg):
                self.text.insert("end", f"[{now}] ", "TIME")
                self.text.insert("end", f"[{tag}] {msg}\n", tag)

        if self.auto_scroll_var.get():
            self.text.see("end")

    # =====================================================================
    # UDP LISTENER & CONNECTION MONITOR
    # =====================================================================

    def udp_listener_loop(self):
        try:
            self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            self.sock.settimeout(1.0)
            self.sock.bind((UDP_IP, UDP_PORT))
            self.ingest_queue.put(("NEW_LINE", {"timestamp": datetime.datetime.now().strftime("%H:%M:%S.%f")[:-3],
                                                "tag": "SYS", "msg": f"UDP socket bound on {UDP_IP}:{UDP_PORT} — listening for ATOMS OS telemetry..."}))
        except Exception as e:
            self.ingest_queue.put(("NEW_LINE", {"timestamp": datetime.datetime.now().strftime("%H:%M:%S.%f")[:-3],
                                                "tag": "CRITICAL", "msg": f"Socket bind FAILED on port {UDP_PORT}: {e} (Run as Admin!)"}))
            return

        while self.running:
            try:
                data, addr = self.sock.recvfrom(2048)
                self.last_packet_time = time.time()
                raw = data.decode("ascii", errors="replace").strip()
                if raw:
                    self.process_telemetry_message(f"[{addr[0]}] {raw}")
            except socket.timeout:
                continue
            except Exception:
                break

    def connection_monitor_loop(self):
        """1Hz Monitor to update ONLINE/OFFLINE connection badge."""
        if not self.running:
            return

        if time.time() - self.last_packet_time < 3.0:
            self.conn_indicator.config(text=" [ONLINE] ", bg="#a6e3a1", fg="#11111b")
        else:
            self.conn_indicator.config(text=" [OFFLINE] ", bg="#313244", fg="#f38ba8")

        self.root.after(1000, self.connection_monitor_loop)

    # =====================================================================
    # ACTION BUTTON COMMAND WORKFLOWS
    # =====================================================================

    def cmd_build(self):
        """Executes build.ps1 in background thread."""
        if self.building_in_progress:
            return
        threading.Thread(target=self._run_build_process, daemon=True).start()

    def _run_build_process(self, on_complete_callback=None):
        self.building_in_progress = True
        self._set_status("STATUS: 🔨 BUILDING KERNEL (build.ps1)...", "#f9e2af")
        self.ingest_queue.put(("NEW_LINE", {"timestamp": datetime.datetime.now().strftime("%H:%M:%S.%f")[:-3],
                                            "tag": "SYS", "msg": "=================================================="}))
        self.ingest_queue.put(("NEW_LINE", {"timestamp": datetime.datetime.now().strftime("%H:%M:%S.%f")[:-3],
                                            "tag": "SYS", "msg": "🔨 EXECUTING POWERSHELL BUILD ENGINE (build.ps1)..."}))

        try:
            cmd = ["powershell", "-ExecutionPolicy", "Bypass", "-File", ".\\build.ps1"]
            proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, bufsize=1)
            
            for line in proc.stdout:
                line_str = line.strip()
                if line_str:
                    self.ingest_queue.put(("NEW_LINE", {"timestamp": datetime.datetime.now().strftime("%H:%M:%S.%f")[:-3],
                                                        "tag": "INFO" if "SUCCESS" in line_str or "OK" in line_str else "SYS",
                                                        "msg": f"[BUILD] {line_str}"}))

            proc.wait()
            if proc.returncode == 0:
                self._set_status("STATUS: ✅ BUILD SUCCESSFUL!", "#a6e3a1")
                self.ingest_queue.put(("NEW_LINE", {"timestamp": datetime.datetime.now().strftime("%H:%M:%S.%f")[:-3],
                                                    "tag": "INFO", "msg": "✅ BUILD SUCCESSFUL!"}))
                if on_complete_callback:
                    on_complete_callback(True)
            else:
                self._set_status("STATUS: ❌ BUILD FAILED!", "#f38ba8")
                self.ingest_queue.put(("NEW_LINE", {"timestamp": datetime.datetime.now().strftime("%H:%M:%S.%f")[:-3],
                                                    "tag": "CRITICAL", "msg": f"❌ BUILD FAILED! (Exit code: {proc.returncode})"}))
                if on_complete_callback:
                    on_complete_callback(False)
        except Exception as e:
            self._set_status(f"STATUS: ❌ BUILD EXCEPTION: {e}", "#f38ba8")
            self.ingest_queue.put(("NEW_LINE", {"timestamp": datetime.datetime.now().strftime("%H:%M:%S.%f")[:-3],
                                                "tag": "CRITICAL", "msg": f"❌ BUILD EXCEPTION: {e}"}))
            if on_complete_callback:
                on_complete_callback(False)
        finally:
            self.building_in_progress = False

    def cmd_build_and_wake(self):
        """Executes build.ps1, then sends Wake-On-LAN packet upon success."""
        def on_build_done(success):
            if success:
                # Start PXE server if needed
                try:
                    subprocess.Popen([sys.executable, os.path.join("tools", "pxe_server.py")],
                                     creationflags=subprocess.CREATE_NEW_CONSOLE)
                except Exception:
                    pass
                # Send WOL
                time.sleep(1.0)
                self.cmd_wake()
                self._set_status("STATUS: ✅ BUILD SUCCESSFUL | WAKE SIGNAL SENT!", "#a6e3a1")

        threading.Thread(target=self._run_build_process, args=(on_build_done,), daemon=True).start()

    def cmd_wake(self):
        """Sends Magic Wake-On-LAN Packet to Target Hardware MAC."""
        try:
            mac_bytes = bytes.fromhex(TARGET_MAC.replace(":", "").replace("-", ""))
            magic_pkt = b"\xff" * 6 + mac_bytes * 16
            with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as s:
                s.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
                s.sendto(magic_pkt, ("192.168.2.255", 9))
                s.sendto(magic_pkt, ("255.255.255.255", 9))
            
            self._dispatch_log("HW", f"⚡ WAKE PACKET SENT TO TARGET MAC [{TARGET_MAC}]")
            self._set_status("STATUS: ⚡ WAKE PACKET SENT", "#cba6f7")
        except Exception as e:
            messagebox.showerror("WOL Exception", str(e))

    def cmd_reboot(self):
        """Sends UDP REBOOT command packet to target ATOMS OS."""
        try:
            with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as s:
                s.sendto(b"REBOOT", (TARGET_IP, UDP_PORT))
            self._dispatch_log("SYS", f"🔄 REBOOT COMMAND SENT TO {TARGET_IP}")
            self._set_status("STATUS: 🔄 REBOOT COMMAND SENT", "#f9e2af")
        except Exception as e:
            messagebox.showerror("Reboot Exception", str(e))

    def cmd_shutdown(self):
        """Sends UDP SHUTDOWN command packet to target ATOMS OS."""
        try:
            with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as s:
                s.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
                for _ in range(3):
                    s.sendto(b"SHUTDOWN", (TARGET_IP, UDP_PORT))
                    s.sendto(b"SHUTDOWN", ("192.168.2.255", UDP_PORT))
                    s.sendto(b"SHUTDOWN", ("255.255.255.255", UDP_PORT))
                    time.sleep(0.05)
            self._dispatch_log("SYS", f"🛑 SHUTDOWN COMMAND SENT TO {TARGET_IP}")
            self._set_status("STATUS: 🛑 SHUTDOWN COMMAND SENT", "#f38ba8")
        except Exception as e:
            messagebox.showerror("Shutdown Exception", str(e))

    def cmd_clear_logs(self):
        """Clears memory buffer and Tkinter text widget."""
        self.log_ring_buffer.clear()
        self.text.delete("1.0", "end")
        self.last_msg_text = None
        self.last_msg_tag = None
        self.last_msg_line_num = None
        self.repeat_count = 1
        self.packets_received = 0
        self.duplicates_collapsed = 0
        self._set_status("STATUS: LOGS CLEARED", "#89dceb")

    def cmd_export_logs(self):
        """Exports currently visible and ring-buffered logs to file."""
        file_path = filedialog.asksaveasfilename(
            defaultextension=".log",
            filetypes=[("Log Files", "*.log"), ("Text Files", "*.txt"), ("All Files", "*.*")],
            title="Export AMDE Log Buffer"
        )
        if not file_path:
            return

        try:
            with open(file_path, "w", encoding="utf-8") as f:
                f.write(f"# ATOMS MATRIX DEBUG ENGINE (AMDE) V1.0 LOG DUMP\n")
                f.write(f"# Export Date: {datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n")
                f.write(f"# Total Ring Records: {len(self.log_ring_buffer)}\n\n")
                
                for r in self.log_ring_buffer:
                    f.write(f"[{r['timestamp']}] [{r['tag']}] {r['msg']}\n")

            self._set_status(f"STATUS: ✅ LOGS EXPORTED TO {os.path.basename(file_path)}", "#a6e3a1")
            messagebox.showinfo("Export Successful", f"Logs successfully exported to:\n{file_path}")
        except Exception as e:
            messagebox.showerror("Export Error", str(e))

    def _set_status(self, text, color):
        self.lbl_status.config(text=text, fg=color)

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
        # Elevate privileges automatically
        ctypes.windll.shell32.ShellExecuteW(
            None, "runas", sys.executable,
            f'"{os.path.abspath(__file__)}"', os.getcwd(), 1)
        sys.exit(0)
    root = tk.Tk()
    app = AMDE_App(root)
    root.mainloop()
