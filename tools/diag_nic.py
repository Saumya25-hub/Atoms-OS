import ctypes
from ctypes import wintypes
import socket
import struct

# Check all network adapters using iphlpapi.dll
class IP_ADAPTER_ADDRESSES(ctypes.Structure):
    pass

# We can use ctypes or simple socket checks.
# First, let's see if ping to 192.168.2.100 works or if ARP can resolve.
import subprocess

print("--- PING TEST 192.168.2.100 ---")
res = subprocess.run(["ping", "-n", "2", "-w", "500", "192.168.2.100"], capture_output=True, text=True)
print(res.stdout)

# Check link status using ctypes GetIfTable or GetIfEntry
class MIB_IFROW(ctypes.Structure):
    _fields_ = [
        ("wszName", wintypes.WCHAR * 256),
        ("dwIndex", wintypes.DWORD),
        ("dwType", wintypes.DWORD),
        ("dwMtu", wintypes.DWORD),
        ("dwSpeed", wintypes.DWORD),
        ("dwPhysAddrLen", wintypes.DWORD),
        ("bPhysAddr", ctypes.c_byte * 8),
        ("dwAdminStatus", wintypes.DWORD),
        ("dwOperStatus", wintypes.DWORD),
        ("dwLastChange", wintypes.DWORD),
        ("dwInOctets", wintypes.DWORD),
        ("dwInUcastPkts", wintypes.DWORD),
        ("dwInNUcastPkts", wintypes.DWORD),
        ("dwInDiscards", wintypes.DWORD),
        ("dwInErrors", wintypes.DWORD),
        ("dwInUnknownProtos", wintypes.DWORD),
        ("dwOutOctets", wintypes.DWORD),
        ("dwOutUcastPkts", wintypes.DWORD),
        ("dwOutNUcastPkts", wintypes.DWORD),
        ("dwOutDiscards", wintypes.DWORD),
        ("dwOutErrors", wintypes.DWORD),
        ("dwOutQLen", wintypes.DWORD),
        ("dwDescrLen", wintypes.DWORD),
        ("bDescr", ctypes.c_char * 256),
    ]

# Get interface index 4 (Realtek)
iphlpapi = ctypes.windll.iphlpapi
row = MIB_IFROW()
row.dwIndex = 4 # Interface 4 from route print
err = iphlpapi.GetIfEntry(ctypes.byref(row))
print(f"GetIfEntry(4) err: {err}")
if err == 0:
    name = row.bDescr[:row.dwDescrLen].decode('ascii', errors='ignore')
    speed_mbps = row.dwSpeed / 1_000_000
    # dwOperStatus: 1=UP, 2=DOWN, 3=TESTING, 4=UNKNOWN, 5=DORMANT, 6=NOT_PRESENT, 7=LOWER_LAYER_DOWN
    oper_status = {1: "UP", 2: "DOWN", 3: "TESTING"}.get(row.dwOperStatus, f"Other({row.dwOperStatus})")
    print(f"Interface 4: {name}")
    print(f"Speed: {speed_mbps} Mbps | Operational Status: {oper_status}")
    print(f"InPackets: {row.dwInUcastPkts + row.dwInNUcastPkts} (Discards: {row.dwInDiscards}, Errors: {row.dwInErrors})")
    print(f"OutPackets: {row.dwOutUcastPkts + row.dwOutNUcastPkts}")

# Check interface index 14 (Wi-Fi)
row_wifi = MIB_IFROW()
row_wifi.dwIndex = 14
err = iphlpapi.GetIfEntry(ctypes.byref(row_wifi))
if err == 0:
    name = row_wifi.bDescr[:row_wifi.dwDescrLen].decode('ascii', errors='ignore')
    oper_status = {1: "UP", 2: "DOWN"}.get(row_wifi.dwOperStatus, f"Other({row_wifi.dwOperStatus})")
    print(f"Interface 14 (Wi-Fi): {name} | OperStatus: {oper_status}")
