#!/usr/bin/env python3
"""
ATOMS OS — AI-(P)DEBUG ENGINE Python Schema
Defines binary packet unpacking, enum decoders, and structured JSON schemas.
"""

import struct
from dataclasses import dataclass, field
from typing import List, Dict, Any, Optional

AIPD_MAGIC = 0x41495044 # "AIPD"
AIPD_UDP_PORT = 9997

HEADER_FORMAT = "<IIIHH" # 16 bytes: magic, session_id, seq_no, record_count, active_profile
HEADER_SIZE = struct.calcsize(HEADER_FORMAT)

RECORD_FORMAT = "<QHBBBBHHIIIH" # 32 bytes: tsc, time_us, subsys, comp, truth, evt, tx_id, tx_type, addr, before, after, res
RECORD_SIZE = struct.calcsize(RECORD_FORMAT)

TRUTH_NAMES = {
    0: "NONE",
    1: "OBSERVED",
    2: "DERIVED",
    3: "INFERRED",
    4: "UNKNOWN"
}

SUBSYS_NAMES = {
    0: "NONE",
    1: "CORE",
    2: "PS2_CONTROLLER",
    3: "PS2_KEYBOARD",
    4: "PS2_MOUSE",
    5: "USB_XHCI",
    6: "NET_RTL8168",
    7: "PMM",
    8: "VMM",
    9: "SCHEDULER",
    10: "COMPOSITOR"
}

COMP_NAMES = {
    0: "NONE",
    1: "8042_PORT60",
    2: "8042_PORT64",
    3: "8042_CONFIG_REG",
    4: "8042_STATUS_REG",
    5: "LPC_CHANNEL1_WIRE",
    6: "LPC_CHANNEL2_WIRE",
    7: "KEYBOARD_DEVICE",
    8: "MOUSE_DEVICE"
}

EVT_NAMES = {
    0: "NONE",
    1: "REG_READ",
    2: "REG_WRITE",
    3: "CMD_DISPATCH",
    4: "RESP_RECEIVED",
    5: "TIMEOUT_EXPIRED",
    6: "STATE_TRANSITION",
    7: "CONFIG_CHANGE",
    8: "TX_BEGIN",
    9: "TX_END"
}

TX_TYPE_NAMES = {
    0: "NONE",
    1: "8042_SELFTEST",
    2: "8042_IFACE1_TEST",
    3: "8042_IFACE2_TEST",
    4: "8042_RECONFIG",
    5: "PS2_RESET_BAT",
    6: "PS2_ENABLE_SCANNING",
    7: "PS2_ECHO_PROBE",
    8: "PS2_SET_LEDS"
}

RES_NAMES = {
    0: "PENDING",
    1: "SUCCESS",
    2: "FAILURE",
    3: "TIMEOUT",
    4: "RESEND",
    5: "BLOCKED"
}

@dataclass
class PacketHeader:
    magic: int
    session_id: int
    sequence_number: int
    record_count: int
    active_profile: int

    @classmethod
    def unpack(cls, data: bytes) -> 'PacketHeader':
        vals = struct.unpack(HEADER_FORMAT, data[:HEADER_SIZE])
        return cls(*vals)

@dataclass
class EventRecord:
    tsc: int
    time_us_delta: int
    subsystem_id: int
    component_id: int
    truth_class: int
    event_type: int
    transaction_id: int
    transaction_type: int
    address_or_port: int
    raw_value_before: int
    raw_value_after: int
    result_status: int

    @classmethod
    def unpack(cls, data: bytes) -> 'EventRecord':
        vals = struct.unpack(RECORD_FORMAT, data[:RECORD_SIZE])
        return cls(*vals)

    def to_dict(self) -> Dict[str, Any]:
        return {
            "tsc": self.tsc,
            "time_us_delta": self.time_us_delta,
            "subsystem": SUBSYS_NAMES.get(self.subsystem_id, f"UNKNOWN_{self.subsystem_id}"),
            "component": COMP_NAMES.get(self.component_id, f"UNKNOWN_{self.component_id}"),
            "truth_class": TRUTH_NAMES.get(self.truth_class, f"UNKNOWN_{self.truth_class}"),
            "event_type": EVT_NAMES.get(self.event_type, f"UNKNOWN_{self.event_type}"),
            "transaction_id": self.transaction_id,
            "transaction_type": TX_TYPE_NAMES.get(self.transaction_type, f"UNKNOWN_{self.transaction_type}"),
            "address_or_port": f"0x{self.address_or_port:X}",
            "raw_value_before": f"0x{self.raw_value_before:X}",
            "raw_value_after": f"0x{self.raw_value_after:X}",
            "result_status": RES_NAMES.get(self.result_status, f"UNKNOWN_{self.result_status}")
        }
