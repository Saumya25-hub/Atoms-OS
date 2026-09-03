import os
import sys
import time
import re
import datetime
from collections import deque, defaultdict

LOG_FILE = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "build", "atoms_live_kernel.log")
REPORT_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "build")

class LiveForensicObserver:
    def __init__(self, log_path, start_at_end=True):
        self.log_path = log_path
        self.start_at_end = start_at_end
        self.running = True
        
        # Ring buffer for time-correlation (stores last 500 events)
        self.event_history = deque(maxlen=500)
        
        # Subsystem metrics
        self.metrics = {
            "heartbeat_count": 0,
            "last_heartbeat_time": None,
            "xhci_packets": 0,
            "mouse_motion_events": 0,
            "cursor_fast_updates": 0,
            "cursor_erases": 0,
            "compositor_frames": 0,
            "compositor_swaps": 0,
            "vram_copy_calls": 0,
            "vram_copy_total_ms": 0.0,
            "syscalls_total": 0,
            "syscall_counts": defaultdict(int),
            "state_transitions": [],
            "current_os_stage": "PRE-BOOT"
        }
        
        # Anomaly detectors state
        self.recent_events_1s = deque()
        self.recent_swaps_1s = deque()
        self.recent_cursor_updates_1s = deque()
        self.recent_syscalls_1s = deque()
        
        self.last_status_print = time.time()
        self.incident_count = 0
        self.detected_anomalies = set()
        
        # Token assembler for split lines
        self.token_buffer = ""

    def parse_line(self, line):
        line = line.strip()
        if not line:
            return None
            
        # Extract timestamp [HH:MM:SS.mmm]
        match = re.match(r'^\[(\d{2}:\d{2}:\d{2}\.\d{3})\]\s*(.*)$', line)
        if match:
            ts_str, content = match.group(1), match.group(2)
            return {"ts": ts_str, "content": content, "raw": line}
        else:
            return {"ts": datetime.datetime.now().strftime("%H:%M:%S.%f")[:-3], "content": line, "raw": line}

    def process_event(self, event):
        ts = event["ts"]
        content = event["content"]
        
        self.event_history.append(event)
        now_sec = time.time()
        self.recent_events_1s.append((now_sec, event))
        
        # Clean older than 1.0s
        while self.recent_events_1s and (now_sec - self.recent_events_1s[0][0]) > 1.0:
            self.recent_events_1s.popleft()
            
        # Detect Stage Transitions
        if "[ROOK] Transitioning to Boot Animation" in content or "ROOK_PAGE_BOOT" in content:
            self.update_stage("BOOT_SPLASH", ts)
        elif "[ROOK] Transitioning to Lock Screen" in content or "ROOK_PAGE_LOCK" in content:
            self.update_stage("ROOK_LOCK", ts)
        elif "ROOK_PAGE_LOGIN" in content or "Entering Interactive Login" in content:
            self.update_stage("ROOK_LOGIN", ts)
        elif "ROOK_PAGE_DESKTOP" in content or "DESKTOP_VISIBLE" in content or "Starting ATOMS OS Enterprise Desktop" in content:
            self.update_stage("ATOMS_DESKTOP", ts)

        # CPU Heartbeat
        if "HEARTBEAT" in content or "diag_heartbeat" in content or "AME_SPINNER" in content:
            self.metrics["heartbeat_count"] += 1
            self.metrics["last_heartbeat_time"] = now_sec

        # USB / xHCI & Pointer Events
        if "XHCI" in content or "USB" in content:
            if "packet" in content.lower() or "event" in content.lower():
                self.metrics["xhci_packets"] += 1
        if "pointer_motion" in content or "POINTER_MOTION" in content or "BSPE_CursorPresenter" in content:
            self.metrics["mouse_motion_events"] += 1
            self.recent_cursor_updates_1s.append(now_sec)

        # Compositor & Graphics
        if "SwapFull" in content or "BOVISUAL_Graphics_SwapFull" in content:
            self.metrics["compositor_swaps"] += 1
            self.recent_swaps_1s.append(now_sec)
        if "BSPE_VRAM_CopyEffectiveDamage" in content:
            self.metrics["vram_copy_calls"] += 1
        if "COMPOSE_COMPLETE" in content or "BCM_CompletePresentation" in content:
            self.metrics["compositor_frames"] += 1

        # Syscalls
        if "[SYSCALL] ENTER ID=" in content or "[SYSCALL] EXIT ID=" in content:
            self.metrics["syscalls_total"] += 1
            self.recent_syscalls_1s.append(now_sec)
            m = re.search(r'ID=(\d+)', content)
            if m:
                sc_id = int(m.group(1))
                self.metrics["syscall_counts"][sc_id] += 1

        # Periodic Anomaly Detection
        self.run_anomaly_checks(ts, content, now_sec)

    def update_stage(self, new_stage, ts):
        if self.metrics["current_os_stage"] != new_stage:
            old_stage = self.metrics["current_os_stage"]
            self.metrics["current_os_stage"] = new_stage
            print(f"\n[{ts}] ⚡ OS STAGE TRANSITION: {old_stage} ➔ {new_stage}", flush=True)

    def run_anomaly_checks(self, current_ts, current_content, now_sec):
        # 1. PRESENTATION STORM CHECK (> 65 full presents in 1 second)
        while self.recent_swaps_1s and (now_sec - self.recent_swaps_1s[0]) > 1.0:
            self.recent_swaps_1s.popleft()
        if len(self.recent_swaps_1s) > 65:
            self.raise_incident(
                "ANOMALY_PRESENTATION_STORM",
                current_ts,
                subsystem="COMPOSITOR / GRAPHICS",
                severity="HIGH",
                observed=f"SwapFull invoked {len(self.recent_swaps_1s)} times in 1.0 second (> 60Hz limit)",
                derived="Compositor is trapped in an uncontrolled presentation loop",
                inferred="Damage tracker is re-arming on every frame completion",
                event_chain="BWE_ComposeFrame ➔ SwapFull ➔ EndComposition ➔ BCM_Process ➔ Repeat",
                root_cause="CONFIRMED"
            )

        # 2. SYSCALL STORM CHECK (> 1000 syscalls in 1 second)
        while self.recent_syscalls_1s and (now_sec - self.recent_syscalls_1s[0]) > 1.0:
            self.recent_syscalls_1s.popleft()
        if len(self.recent_syscalls_1s) > 1000:
            self.raise_incident(
                "ANOMALY_SYSCALL_STORM",
                current_ts,
                subsystem="SYSCALL / USERSPACE",
                severity="MEDIUM",
                observed=f"Syscalls arriving at {len(self.recent_syscalls_1s)}/sec",
                derived="Ring 3 user task is in a non-yielding spin loop",
                inferred="Task polling events without sleeping",
                event_chain="SYS_GUI_POLL_EVENT ➔ SYS_GETPID ➔ Repeat",
                root_cause="SUSPECTED"
            )

        # 3. PCIe BUS STALL (> 150ms single copy)
        m_copy = re.search(r'BSPE_VRAM_CopyEffectiveDamage\s*->\s*Avg:\s*([\d\.]+)\s*ms', current_content)
        if m_copy:
            avg_ms = float(m_copy.group(1))
            if avg_ms > 120.0:
                self.raise_incident(
                    "ANOMALY_PCIE_BUS_SATURATION",
                    current_ts,
                    subsystem="GRAPHICS / PCIE",
                    severity="HIGH",
                    observed=f"BSPE_VRAM_CopyEffectiveDamage average duration is {avg_ms} ms per frame",
                    derived="PCIe bus is saturated copying full uncompressed 2560x1600 frame buffer",
                    inferred="Partial damage clipping is inactive or full damage fallback triggered",
                    event_chain="BCM_Process ➔ VRAM_Copy ➔ Bus Wait ➔ Lag",
                    root_cause="SUSPECTED"
                )

    def raise_incident(self, incident_type, timestamp, subsystem, severity, observed, derived, inferred, event_chain, root_cause):
        if incident_type in self.detected_anomalies:
            return  # Avoid duplicate floods
        self.detected_anomalies.add(incident_type)
        self.incident_count += 1
        
        # Time-correlated reconstruction (T-100ms to T)
        context_events = list(self.event_history)[-15:]
        
        print("\n" + "=" * 78, flush=True)
        print(f"🔴 ANOMALY DETECTED — {incident_type}", flush=True)
        print("=" * 78, flush=True)
        print(f"Incident ID: INC-{self.incident_count:04d}")
        print(f"Timestamp:   {timestamp}")
        print(f"Hardware:    ASUS B750M-K (Intel i3-14100F, 2560x1600 UEFI GOP)")
        print(f"Subsystem:   {subsystem}")
        print(f"Severity:    {severity}")
        print(f"\nOBSERVED:\n  {observed}")
        print(f"\nDERIVED:\n  {derived}")
        print(f"\nINFERRED:\n  {inferred}")
        print(f"\nEVENT CHAIN:\n  {event_chain}")
        print(f"\nTIME-CORRELATED RECONSTRUCTION (T-15 events to T):")
        for ev in context_events:
            print(f"  [{ev['ts']}] {ev['content']}")
        print(f"\nROOT CAUSE CLASSIFICATION: {root_cause}")
        print("=" * 78 + "\n", flush=True)

    def print_heartbeat_status(self):
        now = time.time()
        if now - self.last_status_print >= 5.0:
            self.last_status_print = now
            ts = datetime.datetime.now().strftime("%H:%M:%S")
            stage = self.metrics["current_os_stage"]
            swaps = len(self.recent_swaps_1s)
            cursor_rate = len(self.recent_cursor_updates_1s)
            sysc_rate = len(self.recent_syscalls_1s)
            
            print(f"[{ts}] [LIVE TELEMETRY] Stage: {stage:<14} | SwapFull: {swaps:3d}/s | Cursor: {cursor_rate:3d}/s | Syscalls: {sysc_rate:4d}/s | Anomalies: {self.incident_count}", flush=True)

    def monitor(self):
        print("=" * 78, flush=True)
        print("  ATOMS OS — AUTONOMOUS LIVE FORENSIC TELEMETRY OBSERVER V1.0", flush=True)
        print("  Target: ASUS B750M-K (Intel i3-14100F, Native UEFI GOP at 2560x1600)")
        print(f"  Log Source: {self.log_path}")
        print("=" * 78, flush=True)
        
        while not os.path.exists(self.log_path):
            print(f"Waiting for log file: {self.log_path} ...", flush=True)
            time.sleep(1)
            
        with open(self.log_path, "r", encoding="utf-8", errors="ignore") as f:
            if self.start_at_end:
                f.seek(0, os.SEEK_END)
                print(f"[OBSERVER] Attached to live stream at end-of-file. Listening for physical ASUS B750M-K boot...", flush=True)
            else:
                print(f"[OBSERVER] Reading entire log history...", flush=True)

            while self.running:
                line = f.readline()
                if line:
                    ev = self.parse_line(line)
                    if ev:
                        self.process_event(ev)
                else:
                    self.print_heartbeat_status()
                    time.sleep(0.05)

if __name__ == "__main__":
    start_at_end = True
    if len(sys.argv) > 1 and sys.argv[1] == "--replay":
        start_at_end = False
    observer = LiveForensicObserver(LOG_FILE, start_at_end=start_at_end)
    try:
        observer.monitor()
    except KeyboardInterrupt:
        print("\n[OBSERVER] Stopped.", flush=True)
