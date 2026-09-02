#!/usr/bin/env python3
"""
ATOMS OS — AI-(P)DEBUG ENGINE Binary Wire Receiver
Listens on UDP port 9997 for binary AIPD forensic packets.
"""

import socket
import struct
import threading
import time
import os
import json
from typing import List, Dict, Any, Optional

import sys
import os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from schema import (
    AIPD_MAGIC, AIPD_UDP_PORT, HEADER_FORMAT, HEADER_SIZE,
    RECORD_FORMAT, RECORD_SIZE, PacketHeader, EventRecord
)

class AIPDReceiver:
    def __init__(self, bind_ip="0.0.0.0", port=AIPD_UDP_PORT):
        self.bind_ip = bind_ip
        self.port = port
        self.sock: Optional[socket.socket] = None
        self.running = False
        self.thread: Optional[threading.Thread] = None
        self.packets: List[PacketHeader] = []
        self.events: List[EventRecord] = []
        self.lock = threading.Lock()
        self.dropped_counter = 0

    def start(self):
        with self.lock:
            if self.running:
                return
            self.running = True
            self.packets = []
            self.events = []
            self.dropped_counter = 0

        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.sock.bind((self.bind_ip, self.port))
        self.sock.settimeout(0.5)

        self.thread = threading.Thread(target=self._listen_loop, daemon=True)
        self.thread.start()
        print(f"[AIPDEBUG-RECEIVER] Listening on {self.bind_ip}:{self.port} (UDP)")

    def stop(self):
        with self.lock:
            self.running = False
        if self.sock:
            try:
                self.sock.close()
            except Exception:
                pass
        if self.thread and self.thread.is_alive():
            self.thread.join(timeout=1.0)
        print(f"[AIPDEBUG-RECEIVER] Stopped. Total events captured: {len(self.events)}")

    def _listen_loop(self):
        last_seq = 0
        while True:
            with self.lock:
                if not self.running:
                    break
            try:
                data, addr = self.sock.recvfrom(2048)
                if len(data) < HEADER_SIZE:
                    continue

                header = PacketHeader.unpack(data[:HEADER_SIZE])
                if header.magic != AIPD_MAGIC:
                    continue

                if last_seq > 0 and header.sequence_number > last_seq + 1:
                    missed = header.sequence_number - (last_seq + 1)
                    self.dropped_counter += missed
                    print(f"[AIPDEBUG-RECEIVER] [WARN] Missed {missed} packets in sequence!")
                last_seq = header.sequence_number

                offset = HEADER_SIZE
                new_events = []
                for _ in range(header.record_count):
                    if offset + RECORD_SIZE > len(data):
                        break
                    evt = EventRecord.unpack(data[offset:offset+RECORD_SIZE])
                    new_events.append(evt)
                    offset += RECORD_SIZE

                with self.lock:
                    self.packets.append(header)
                    self.events.extend(new_events)

            except socket.timeout:
                continue
            except Exception as e:
                with self.lock:
                    if not self.running:
                        break
                print(f"[AIPDEBUG-RECEIVER] Error: {e}")

    def get_events_dict(self) -> List[Dict[str, Any]]:
        with self.lock:
            return [e.to_dict() for e in self.events]

    def save_dump(self, dest_dir: str):
        os.makedirs(dest_dir, exist_ok=True)
        events_dict = self.get_events_dict()
        out_json = os.path.join(dest_dir, "aipd_events.json")
        with open(out_json, "w", encoding="utf-8") as f:
            json.dump({
                "packet_count": len(self.packets),
                "event_count": len(events_dict),
                "dropped_counter": self.dropped_counter,
                "events": events_dict
            }, f, indent=2)
        print(f"[AIPDEBUG-RECEIVER] [OK] Saved {len(events_dict)} events to {out_json}")

if __name__ == "__main__":
    rec = AIPDReceiver()
    rec.start()
    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        rec.stop()
