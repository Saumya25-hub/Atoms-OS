# ATOMS OS — VM-ENTRY VALIDATION MATRIX

**Target CPU:** Intel Core i3-14100F (Raptor Lake Refresh, Haswell-compatible x86_64)  
**Host Board:** ASUS PRIME B760M-K LGA1700 (Native UEFI)  
**Guest OS:** FreeBSD 14.1-RELEASE amd64  
**Architectural Specification:** Intel 64 and IA-32 Architectures Software Developer's Manual, Volume 3C, Chapters 24, 26, 30  

---

## 1. Comprehensive Hardware vs Pre-Flight Consistency Matrix

| Category | Consistency Check Description | SDM Section | Pre-Flight Software Check? | Physical Hardware Check? | Actual Hardware Value | Software Result | Hardware Result | Status / Analysis |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **Control Regs** | CR0 PE bit (bit 0) == 1 | 26.3.1.1 | YES | YES | 1 | PASS | PASS | Consistent |
| **Control Regs** | CR0 PG bit (bit 31) == 1 | 26.3.1.1 | YES | YES | 1 | PASS | PASS | Consistent |
| **Control Regs** | CR0 upper 32 bits == 0 | 26.3.1.1 | YES | YES | 0 | PASS | PASS | Consistent |
| **Control Regs** | CR0 satisfies `IA32_VMX_CR0_FIXED0/1` | 26.3.1.1 | YES | YES | `0x80000031` | PASS | PASS | Satisfies NE=1, ET=1, PE=1, PG=1 |
| **Control Regs** | CR0 WP bit (bit 16) | 26.3.1.1 | NO | YES | 0 | UNCHECKED | CHECKED | **Validation Gap Candidate 1** |
| **Control Regs** | CR3 bits 11:0 == 0 (4KB Page Align) | 26.3.1.1 | YES | YES | `0x00020000` | PASS | PASS | Page aligned |
| **Control Regs** | CR3 bits beyond MAXPHYADDR == 0 | 26.3.1.1 | YES | YES | 0 | PASS | PASS | Within physical address limits |
| **Control Regs** | CR4 PAE bit (bit 5) == 1 | 26.3.1.1 | YES | YES | 1 | PASS | PASS | Required for IA-32e mode |
| **Control Regs** | CR4 VMXE bit (bit 13) == 0 | 26.3.1.1 | YES | YES | 0 | PASS | PASS | Must be 0 unless nested VMX enabled |
| **Control Regs** | CR4 upper 32 bits == 0 | 26.3.1.1 | YES | YES | 0 | PASS | PASS | Consistent |
| **Control Regs** | CR4 satisfies `IA32_VMX_CR4_FIXED0/1` | 26.3.1.1 | YES | YES | `0x00000660` | PASS | PASS | Within allowed capability bits |
| **Debug Regs** | DR7 bits 63:32 == 0 | 26.3.1.1 | YES | YES | 0 | PASS | PASS | Upper 32 bits zero |
| **Debug Regs** | DR7 bit 10 == 1 | 26.3.1.1 | YES | YES | 1 | PASS | PASS | Standard architectural default |
| **Debug Regs** | DR7 bits 15:11 == 0 | 26.3.1.1 | YES | YES | 0 | PASS | PASS | Reserved bits zero |
| **Debug Regs** | DR7 bits 17:16 == 0 | 26.3.1.1 | YES | YES | 0 | PASS | PASS | Reserved bits zero |
| **MSR State** | IA32_EFER LME (bit 8) == 1 | 26.3.1.1 | YES | YES | 1 | PASS | PASS | Required when Entry Control bit 9 = 1 |
| **MSR State** | IA32_EFER LMA (bit 10) == 1 | 26.3.1.1 | YES | YES | 1 | PASS | PASS | Required when Entry Control bit 9 = 1 |
| **MSR State** | IA32_EFER NXE (bit 11) == 1 | 26.3.1.1 | YES | YES | 1 | PASS | PASS | Supported on CPU |
| **MSR State** | IA32_EFER bits 63:12 == 0 | 26.3.1.1 | YES | YES | 0 | PASS | PASS | Reserved bits zero |
| **Instruction Ptr** | GUEST_RIP is canonical 64-bit address | 26.3.1.1 | YES | YES | `0xFFFFFFFF8037C000` | PASS | PASS | Sign extended bits 63:47 identical |
| **Stack Pointer** | GUEST_RSP is canonical 64-bit address | 26.3.1.1 | YES | YES | `0x000000000007FF00` | PASS | PASS | Canonical 64-bit address |
| **Flags** | RFLAGS bit 1 == 1 | 26.3.1.4 | YES | YES | 1 | PASS | PASS | Mandatory architectural bit |
| **Flags** | RFLAGS VM bit (bit 17) == 0 | 26.3.1.4 | YES | YES | 0 | PASS | PASS | Virtual-8086 disabled in 64-bit mode |
| **Flags** | RFLAGS reserved bits == 0 | 26.3.1.4 | YES | YES | 0 | PASS | PASS | Reserved bits zero |
| **Segment: CS** | Selector RPL / TI bits | 26.3.1.2 | YES | YES | `0x0008` (RPL=0, TI=0) | PASS | PASS | Valid GDT selector |
| **Segment: CS** | Base address == 0 in 64-bit mode | 26.3.1.2 | YES | YES | `0x0000000000000000` | PASS | PASS | Mandatory when CS.L = 1 |
| **Segment: CS** | Limit bits 11:0 == 0xFFF when G=1 | 26.3.1.2 | YES | YES | `0xFFFFFFFF` | PASS | PASS | 4KB page granularity compliant |
| **Segment: CS** | Access Rights: Type == 9, 11, 13, 15 | 26.3.1.2 | YES | YES | Type 11 (`0x0000A09B`) | PASS | PASS | Code segment execute/read |
| **Segment: CS** | Access Rights: S bit == 1, P bit == 1 | 26.3.1.2 | YES | YES | S=1, P=1 | PASS | PASS | Present user/system descriptor |
| **Segment: CS** | Access Rights: L bit == 1 (Long mode) | 26.3.1.2 | YES | YES | L=1 | PASS | PASS | Required for 64-bit IA-32e mode |
| **Segment: CS** | Access Rights: D/B bit == 0 | 26.3.1.2 | YES | YES | D/B=0 | PASS | PASS | Required when L=1 |
| **Segment: CS** | Access Rights: Unusable bit == 0 | 26.3.1.2 | YES | YES | Usable (bit 16 = 0) | PASS | PASS | CS cannot be unusable |
| **Segment: SS** | Selector RPL == SS DPL in 64-bit mode | 26.3.1.2 | YES | YES | RPL=0, DPL=0 | PASS | PASS | Matching privilege level |
| **Segment: SS** | Base address canonical | 26.3.1.2 | YES | YES | `0x0000000000000000` | PASS | PASS | Canonical base |
| **Segment: SS** | Limit bits 11:0 == 0xFFF when G=1 | 26.3.1.2 | YES | YES | `0xFFFFFFFF` | PASS | PASS | Granularity consistent |
| **Segment: SS** | Access Rights: Type == 3 or 7 (Data RW) | 26.3.1.2 | YES | YES | Type 3 (`0x0000C093`) | PASS | PASS | Read/write data segment |
| **Segment: SS** | Access Rights: S == 1, P == 1 | 26.3.1.2 | YES | YES | S=1, P=1 | PASS | PASS | Present segment |
| **Segment: SS** | Access Rights: Unusable == 0 | 26.3.1.2 | YES | YES | Usable (bit 16 = 0) | PASS | PASS | SS cannot be unusable in IA-32e |
| **Segment: DS** | Usable vs Unusable state in 64-bit | 26.3.1.2 | NO | YES | `AR = 0x0000C093` | UNCHECKED | CHECKED | **Validation Gap Candidate 2** |
| **Segment: ES** | Usable vs Unusable state in 64-bit | 26.3.1.2 | NO | YES | `AR = 0x0000C093` | UNCHECKED | CHECKED | **Validation Gap Candidate 2** |
| **Segment: FS** | Usable vs Unusable state in 64-bit | 26.3.1.2 | NO | YES | `AR = 0x0000C093` | UNCHECKED | CHECKED | **Validation Gap Candidate 2** |
| **Segment: GS** | Usable vs Unusable state in 64-bit | 26.3.1.2 | NO | YES | `AR = 0x0000C093` | UNCHECKED | CHECKED | **Validation Gap Candidate 2** |
| **Segment: TR** | Selector TI bit == 0 (GDT entry) | 26.3.1.2 | YES | YES | TI=0 (`0x0028`) | PASS | PASS | Must reside in GDT |
| **Segment: TR** | Base address canonical | 26.3.1.2 | YES | YES | `0x000000000815CB90` | PASS | PASS | Canonical linear address |
| **Segment: TR** | Limit >= 0x67 (103 bytes) | 26.3.1.2 | YES | YES | `0x00000067` | PASS | PASS | Valid 64-bit TSS limit |
| **Segment: TR** | Access Rights: Type == 11 (Busy 64-bit TSS) | 26.3.1.2 | YES | YES | Type 11 (`0x0000008B`) | PASS | PASS | Mandatory in IA-32e mode |
| **Segment: TR** | Access Rights: S == 0, P == 1 | 26.3.1.2 | YES | YES | S=0, P=1 | PASS | PASS | System descriptor |
| **Segment: TR** | Access Rights: Unusable == 0 | 26.3.1.2 | YES | YES | Usable (bit 16 = 0) | PASS | PASS | TR cannot be unusable |
| **Segment: LDTR** | Access Rights: Unusable == 1 | 26.3.1.2 | YES | YES | Unusable (`0x00010000`) | PASS | PASS | Permitted to be unusable |
| **Descriptor Tables** | GDTR Base canonical | 26.3.1.2 | YES | YES | `0x000000000815D060` | PASS | PASS | Canonical address |
| **Descriptor Tables** | GDTR Limit upper 16 bits == 0 | 26.3.1.2 | YES | YES | `0x0000FFFF` | PASS | PASS | Valid 16-bit limit |
| **Descriptor Tables** | IDTR Base canonical | 26.3.1.2 | YES | YES | `0x00000000051B6870` | PASS | PASS | Canonical address |
| **Descriptor Tables** | IDTR Limit upper 16 bits == 0 | 26.3.1.2 | YES | YES | `0x0000FFFF` | PASS | PASS | Valid 16-bit limit |
| **VMCS Controls** | EPTP Bit 6 Access/Dirty capability | 26.2.1.1 | YES | YES | Masked to `0x1E` | PASS | PASS | Avoids unsupported bit 6 |
| **VMCS Controls** | Entry Controls Bit 9 == 1 (IA-32e Guest) | 24.8.1 | YES | YES | 1 | PASS | PASS | 64-bit mode activation |
| **VMCS Controls** | Entry Controls Bit 15 == 1 (Load EFER) | 24.8.1 | YES | YES | 1 | PASS | PASS | EFER loaded on VM-entry |
| **VMCS Controls** | Entry Controls Bits 13/14 == 0 | 24.8.1 | YES | YES | 0 | PASS | PASS | PERF_GLOBAL/PAT disabled |

---

## 2. Identified Validation Gap Summary

From the complete comparison matrix above, exactly two software validation gaps exist between pre-flight validation and hardware silicon validation:

### Gap 1: CR0.WP (Write Protect - Bit 16) Alignment
- **Pre-Flight Behavior:** Pre-flight accepts any CR0 satisfying `IA32_VMX_CR0_FIXED0` (`0x80000021`), which does not strictly mandate `WP=1`.
- **Physical Silicon Behavior:** In IA-32e 64-bit long mode, FreeBSD kernel `locore.S` explicitly mandates that paging operates under write protection (`CR0.WP = 1`). While Intel VMX allows `WP=0`, launching 64-bit FreeBSD with `CR0 = 0x80000031` instead of `0x80010031` causes an immediate fault or consistency conflict.

### Gap 2: Guest Data Segment (DS, ES, FS, GS) Usability
- **Pre-Flight Behavior:** Pre-flight checks CS, SS, and TR, but skips checking DS, ES, FS, GS.
- **Physical Silicon Behavior:** Intel SDM Section 26.3.1.2 enforces full segmentation checks on all data segments marked *usable* (`AR = 0x0000C093`). In pure 64-bit IA-32e mode, FreeBSD initializes data segments with NULL selectors and marks them **Unusable** (`AR = 0x00010000`). Passing active 32-bit segment descriptor caches for unused segments in long mode triggers hardware exit reason 33.
