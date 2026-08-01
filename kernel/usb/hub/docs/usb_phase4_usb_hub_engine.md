# SIGNATURES OS — USB PHASE 4: USB HUB ENGINE (UHE) SPECIFICATION

## Overview
The **USB Hub Engine (UHE)** is the central topology manager of the Signatures OS USB subsystem. It governs device discovery, port power state management, reset sequencing, hot-plug/hot-remove dispatching, parent-child hierarchy building, power budget auditing, and error recovery across all USB 1.1, USB 2.0, and USB 3.x SuperSpeed hubs.

---

## 🏛️ Subsystem Architecture

```
                               ┌─────────────────────────────┐
                               │  USB Controller Manager     │
                               └──────────────┬──────────────┘
                                              │
                               ┌──────────────▼──────────────┐
                               │   USB Core & URB Engine     │
                               └──────────────┬──────────────┘
                                              │
                               ┌──────────────▼──────────────┐
                               │    USB Hub Engine (UHE)     │
                               └──────────────┬──────────────┘
                                              │
                   ┌──────────────────────────┼──────────────────────────┐
                   │                          │                          │
        ┌──────────▼──────────┐    ┌──────────▼──────────┐    ┌──────────▼──────────┐
        │ Port State Machine  │    │  Topology Engine    │    │ Power Budget Manager│
        └─────────────────────┘    └─────────────────────┘    └─────────────────────┘
```

---

## 🔄 11-Stage Port State Machine

```
DISCONNECTED -> CONNECTED -> POWERED -> RESETTING -> ENUMERATING -> CONFIGURED -> READY -> SUSPENDED -> RESUMED -> REMOVED -> ERROR
```

---

## 📊 Telemetry & JSON Schema

```json
{
  "uhe_subsystem_report": {
    "total_hubs": 2,
    "total_ports": 8,
    "active_ports": 4,
    "connected_devices": 3,
    "max_hub_depth": 7,
    "status": "PRODUCTION_READY"
  }
}
```
