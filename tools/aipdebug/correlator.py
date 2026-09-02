#!/usr/bin/env python3
"""
ATOMS OS — AI-(P)DEBUG ENGINE Correlator
Groups low-level event records into high-level transactions and state graphs.
"""

from typing import List, Dict, Any

class AIPDCorrelator:
    @staticmethod
    def correlate_transactions(events: List[Dict[str, Any]]) -> List[Dict[str, Any]]:
        transactions_map: Dict[int, Dict[str, Any]] = {}
        standalone_events = []

        for evt in events:
            tx_id = evt.get("transaction_id", 0)
            if tx_id == 0:
                standalone_events.append(evt)
                continue

            if tx_id not in transactions_map:
                transactions_map[tx_id] = {
                    "transaction_id": tx_id,
                    "transaction_type": evt.get("transaction_type", "UNKNOWN"),
                    "start_tsc": evt.get("tsc", 0),
                    "end_tsc": evt.get("tsc", 0),
                    "duration_us": 0,
                    "result": "PENDING",
                    "events": [],
                    "state_transitions": {}
                }

            tx = transactions_map[tx_id]
            tx["events"].append(evt)
            tx["end_tsc"] = evt.get("tsc", 0)
            if evt.get("time_us_delta", 0) > tx["duration_us"]:
                tx["duration_us"] = evt.get("time_us_delta", 0)

            evt_type = evt.get("event_type", "")
            if evt_type == "TX_END":
                tx["result"] = evt.get("result_status", "UNKNOWN")

            # Track state changes
            if evt_type in ("CONFIG_CHANGE", "STATE_TRANSITION"):
                addr = evt.get("address_or_port", "REG")
                tx["state_transitions"][addr] = {
                    "before": evt.get("raw_value_before"),
                    "after": evt.get("raw_value_after")
                }

        results = list(transactions_map.values())
        results.sort(key=lambda x: x["start_tsc"])
        return results
