# tools/update_infoweb.py
"""
Script to update ATOMS-OS-INFOWEB with FreeBSD 14.1 Interactive Keyboard Milestone.
"""

def update_os_data():
    path = "ATOMS-OS-INFOWEB/src/data/osData.js"
    with open(path, "r", encoding="utf-8") as f:
        content = f.read()

    # 1. Update status
    content = content.replace(
        'status: "v2.8.0-vmx-freebsd-live (Bare-Metal Shell Certified)"',
        'status: "v2.9.0-freebsd-keyboard-certified (Bare-Metal Shell & Interactive Keyboard Certified)"'
    )

    # 2. Update b760m-raptorlake certifiedFeatures
    old_b760m_features = '''    certifiedFeatures: [
      "Bare-Metal Intel VT-x (VMX) Root Operation & EPT SLAT Paging",
      "Physical LGA1700 VM-Entry & Clean VM-Exit Execution (500k+ exits)",
      "Native NVMe Gen4 Storage Controller + GPT + NTFS Partition Parsing",
      "Realtek R8168 & RTL8125 2.5GbE PCIe NIC Telemetry",
      "xHCI 1.2 USB 3.0 Controller & Physical HID Keyboard Ring Dequeue"
    ]'''

    new_b760m_features = '''    certifiedFeatures: [
      "Bare-Metal Intel VT-x (VMX) Root Operation & EPT SLAT Paging",
      "Physical LGA1700 VM-Entry & Clean VM-Exit Execution (500k+ exits)",
      "FreeBSD 14.1 Interactive Keyboard & ACPI DSDT (PNP0303/PNP0501) Certified",
      "Native NVMe Gen4 Storage Controller + GPT + NTFS Partition Parsing",
      "Realtek R8168 & RTL8125 2.5GbE PCIe NIC Telemetry",
      "xHCI 1.2 USB 3.0 Controller & Physical HID Keyboard Ring Dequeue"
    ]'''
    assert old_b760m_features in content, "old_b760m_features not found"
    content = content.replace(old_b760m_features, new_b760m_features)

    # 3. Update CORE_ENGINES hypervisor entry
    old_hypervisor_engine = '''    id: "hypervisor",
    name: "Intel VT-x Type-1 Micro-Hypervisor",
    codeName: "VMX Core Engine",
    authority: "kernel/core/hypervisor/ & arch/x86_64/vmx/",
    status: "CERTIFIED (v2.7.0-vmx-stable)",
    tag: "Bare-Metal Silicon",
    description: "Hardware-accelerated Ring 0 hypervisor running in VMX root operation on bare metal Intel silicon without intermediate host OS. Implements hardware VMCS lifecycle, Extended Page Tables (EPT) Second Level Address Translation (SLAT), and high-volume exit handling.",
    highlights: [
      "Physical Intel Core i3-14100F certification with VMLAUNCH (CF=0, ZF=0)",
      "Exit Reason 0x0A (CPUID) and Port 0xCF9 guest reset autopsy handled natively",
      "High-volume exit handling: 500,000+ bare-metal physical exits, 2.0M+ guest execution exits",
      "Validated against all 18 Intel SDM Vol 3C Sec 26.3 VMCS invariants"
    ],'''

    new_hypervisor_engine = '''    id: "hypervisor",
    name: "Intel VT-x Type-1 Micro-Hypervisor",
    codeName: "VMX Core Engine",
    authority: "kernel/core/hypervisor/ & arch/x86_64/vmx/",
    status: "CERTIFIED (v2.9.0-freebsd-keyboard-certified)",
    tag: "Bare-Metal Silicon",
    description: "Hardware-accelerated Ring 0 hypervisor running in VMX root operation on bare metal Intel silicon without intermediate host OS. Implements hardware VMCS lifecycle, Extended Page Tables (EPT) Second Level Address Translation (SLAT), synthetic ACPI DSDT device enumeration, and virtual i8042 PS/2 controller.",
    highlights: [
      "Physical Intel Core i3-14100F certification with VMLAUNCH (CF=0, ZF=0)",
      "FreeBSD 14.1 guest interactive shell with Intel IASL validated ACPI DSDT (PNP0303/PNP0501)",
      "Virtual i8042 PS/2 controller, IRQ1 interrupt injection, and USB HID lock LED synchronization",
      "High-volume exit handling: 500,000+ bare-metal physical exits, 2.0M+ guest execution exits",
      "Validated against all 18 Intel SDM Vol 3C Sec 26.3 VMCS invariants"
    ],'''
    assert old_hypervisor_engine in content, "old_hypervisor_engine not found"
    content = content.replace(old_hypervisor_engine, new_hypervisor_engine)

    # 4. Update SUBSYSTEM_REGISTRY
    old_subsys = '''  { subsystem: "Intel VT-x Type-1 Hypervisor (v2.8.0)", tier: "Virtualization", status: "STABLE", path: "kernel/core/hypervisor/" },
  { subsystem: "FreeBSD Guest Virtualization (Live Shell)", tier: "Virtualization", status: "STABLE", path: "kernel/core/hypervisor/" },'''

    new_subsys = '''  { subsystem: "Intel VT-x Type-1 Hypervisor (v2.9.0)", tier: "Virtualization", status: "CERTIFIED", path: "kernel/core/hypervisor/" },
  { subsystem: "FreeBSD Guest Virtualization (Interactive Shell)", tier: "Virtualization", status: "CERTIFIED", path: "kernel/core/hypervisor/" },'''
    assert old_subsys in content, "old_subsys not found"
    content = content.replace(old_subsys, new_subsys)

    # 5. Add to GENESIS_HISTORY
    old_history_entry = '''    commit: "48bc2bd",
    date: "2026-09-24",
    author: "Saumya25-hub",
    title: "Kernel v2.7.0-vmx-stable: Bare-Metal VT-x & xHCI Input Pipeline",
    phase: "Current Stable Release",
    description: "Unified Type-1 micro-hypervisor, native xHCI USB 3.0 controller, and BCM desktop compositor into master certified baseline.",
    verifiedAuthor: "Saumya Chaudhari"
  }'''

    new_history_entry = old_history_entry + ''',
  {
    commit: "360a4f3",
    date: "2026-10-30",
    author: "Saumya25-hub",
    title: "FreeBSD 14.1 Interactive Keyboard & ACPI DSDT Certification",
    phase: "Hypervisor Phase",
    description: "Solved FreeBSD ACPI legacy ISA probe suppression via synthetic DSDT (PNP0303/PNP0501) verified by Intel IASL, virtual i8042 PS/2 controller, VMCS interrupt window IRQ1 injection, and bidirectional USB HID lock LED synchronization.",
    verifiedAuthor: "Saumya Chaudhari"
  }'''
    assert old_history_entry in content, "old_history_entry not found"
    content = content.replace(old_history_entry, new_history_entry)

    # 6. Add to CERTIFICATION_MATRIX
    old_cert_matrix = '  { subsystem: "Intel VT-x Hypervisor", target: "Physical Core i3-14100F", status: "CERTIFIED PASS", evidence: "ATOMS_VMX_HARDWARE_CERTIFICATION.md (500k+ exits)" },'
    new_cert_matrix = old_cert_matrix + '\n  { subsystem: "FreeBSD Interactive Kbd", target: "ASUS B760M-K (i3-14100F)", status: "CERTIFIED PASS", evidence: "vmstat -i (irq1: atkbd0 14), live typing echo" },'
    assert old_cert_matrix in content, "old_cert_matrix not found"
    content = content.replace(old_cert_matrix, new_cert_matrix)

    with open(path, "w", encoding="utf-8", newline="\n") as f:
        f.write(content)
    print("osData.js updated successfully!")

def update_updates_page():
    path = "ATOMS-OS-INFOWEB/src/components/UpdatesPage.jsx"
    with open(path, "r", encoding="utf-8") as f:
        content = f.read()

    target = "  const releases = [\n"
    new_release = '''  const releases = [
    {
      version: "v2.9.0-freebsd-keyboard-certified",
      date: "October 30, 2026",
      tag: "Interactive Hypervisor Milestone",
      title: "FreeBSD 14.1 Interactive Keyboard & ACPI DSDT Certification on Bare-Metal Silicon",
      summary: "Major bare-metal milestone achieved: Unlocked 100% interactive input inside FreeBSD 14.1 guest shell running under our Type-1 Micro-Hypervisor on physical Intel LGA1700 silicon (Core i3-14100F). Overcame FreeBSD's ACPI legacy ISA probe suppression, engineered synthetic ACPI DSDT AML validated via Intel IASL with zero errors/warnings, virtualized i8042 PS/2 controller on ports 0x60/0x64, synchronized physical USB HID keyboard LEDs, and enabled seamless live shell typing.",
      bulletPoints: [
        "Identified ACPI ISA probe suppression: Under FreeBSD's default ACPI boot mode (acpi0 under nexus0), legacy ISA probing is completely disabled unless enumerated by ACPI. Because our hypervisor PCI bus lacked a PCI-to-ISA bridge (class 06-01) and the DSDT lacked PNP0A00, FreeBSD never created isa0, ignoring all static ISA hints.",
        "Synthetic ACPI DSDT Device Nodes: Compiled and integrated 325-byte AML bytecode under \\\\_SB containing Device (KBD) (PNP0303, IO 0x60/0x64, IRQ 1) and Device (COM1) (PNP0501, IO 0x3F8-0x3FF, IRQ 4), validated cleanly via Intel IASL compiler with 0 errors and 0 warnings.",
        "Virtual i8042 PS/2 Controller & Safe IRQ1 Injection: Built full i8042 state machine with circular FIFO, status bit 0 (OBF), controller commands, and Scancode Set 1/2 translation. Eliminated fallback injection trap 30 panic by injecting IRQ1 only after FreeBSD programs IOAPIC RTE[1] with vector >= 0x20.",
        "Bi-Directional USB HID Lock LED Synchronization: Translated guest 0xED LED commands between PS/2 bitmask and USB HID Boot Keyboard format, dispatching hardware SET_REPORT packets to physical keyboard LEDs and preserving states across Ctrl+Alt+G focus transitions.",
        "Physical Bare-Metal Proof: vmstat -i inside FreeBSD guest confirms 'irq1: atkbd0 total 14 rate 0', and live interactive commands executed cleanly in the root shell ('echo hellow', 'echo dishant like shock mt hona')."
      ]
    },
'''
    assert target in content, "releases array not found in UpdatesPage.jsx"
    content = content.replace(target, new_release)

    with open(path, "w", encoding="utf-8", newline="\n") as f:
        f.write(content)
    print("UpdatesPage.jsx updated successfully!")

if __name__ == "__main__":
    update_os_data()
    update_updates_page()
