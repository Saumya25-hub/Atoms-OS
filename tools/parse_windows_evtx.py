#!/usr/bin/env python3
import os
import xml.etree.ElementTree as ET
import Evtx.Evtx as evtx

EVTX_PATH = os.path.join(os.path.dirname(__file__), "..", "artifacts", "windows_forensic", "System.evtx")
OUT_TXT = os.path.join(os.path.dirname(__file__), "..", "artifacts", "windows_forensic", "evtx_analysis_summary.txt")

def analyze():
    print(f"Opening {EVTX_PATH}...")
    records = []
    errors = []
    boot_events = []
    
    with evtx.Evtx(EVTX_PATH) as log:
        for idx, record in enumerate(log.records()):
            try:
                xml_str = record.xml()
                root = ET.fromstring(xml_str)
                ns = {'ns': 'http://schemas.microsoft.com/win/2004/08/events/event'}
                system = root.find('ns:System', ns)
                if system is None:
                    continue
                
                prov = system.find('ns:Provider', ns)
                prov_name = prov.get('Name', '') if prov is not None else ''
                
                eid_elem = system.find('ns:EventID', ns)
                eid = eid_elem.text if eid_elem is not None else ''
                
                tc = system.find('ns:TimeCreated', ns)
                t_str = tc.get('SystemTime', '') if tc is not None else ''
                
                lvl_elem = system.find('ns:Level', ns)
                lvl = lvl_elem.text if lvl_elem is not None else '0'
                
                event_data = {}
                ed = root.find('ns:EventData', ns)
                if ed is not None:
                    for data_node in ed.findall('ns:Data', ns):
                        d_name = data_node.get('Name', '')
                        d_val = data_node.text or ''
                        if d_name:
                            event_data[d_name] = d_val
                
                item = {
                    'idx': idx,
                    'time': t_str,
                    'provider': prov_name,
                    'eid': eid,
                    'level': lvl,
                    'data': event_data,
                    'xml': xml_str
                }
                
                records.append(item)
                
                # Level 1 = Critical, Level 2 = Error
                if lvl in ('1', '2'):
                    errors.append(item)
                
                if 'Kernel-Boot' in prov_name or 'Kernel-General' in prov_name or 'BugCheck' in prov_name or eid in ('1001', '41', '1', '12', '13', '20'):
                    boot_events.append(item)
            except Exception as e:
                continue

    print(f"Total Events Parsed: {len(records)}")
    print(f"Critical & Error Events: {len(errors)}")
    print(f"Boot & Kernel Lifecycle Events: {len(boot_events)}")

    with open(OUT_TXT, 'w', encoding='utf-8') as f:
        f.write(f"ATOMS OS FORENSIC LOG ANALYSIS -- System.evtx\n")
        f.write(f"Total Events: {len(records)} | Critical/Error: {len(errors)}\n")
        f.write("=" * 80 + "\n\n")
        
        f.write("--- LAST 20 EVENTS CHRONOLOGICALLY ---\n")
        for r in records[-20:]:
            f.write(f"[{r['time']}] Level={r['level']} Provider={r['provider']} EID={r['eid']} Data={r['data']}\n")
        f.write("\n" + "=" * 80 + "\n\n")
        
        f.write("--- LAST 20 CRITICAL / ERROR EVENTS (LEVEL 1 & 2) ---\n")
        for r in errors[-20:]:
            f.write(f"[{r['time']}] Provider={r['provider']} EID={r['eid']}\n")
            f.write(f"   Data: {r['data']}\n")
        f.write("\n" + "=" * 80 + "\n\n")

        f.write("--- LAST 20 BOOT / SHUTDOWN / KERNEL LIFECYCLE EVENTS ---\n")
        for r in boot_events[-20:]:
            f.write(f"[{r['time']}] Provider={r['provider']} EID={r['eid']} Data={r['data']}\n")

    print(f"Summary written to {OUT_TXT}")

if __name__ == '__main__':
    analyze()
