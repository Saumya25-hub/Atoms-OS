import struct
import subprocess
import os
import re

# Read s_dsdt_aml from virtual_platform.c
with open(r"kernel\core\hypervisor\src\virtual_platform.c", "r") as f:
    code = f.read()

start_marker = "#if CONFIG_ACPI_ENABLE_KBD_UART_DSDT"
end_marker = "#else"
idx1 = code.find(start_marker)
idx2 = code.find(end_marker, idx1)

block = code[idx1:idx2]
aml_bytes = bytes([int(x, 16) for x in re.findall(r"0x[0-9a-fA-F]{2}", block)])
print(f"Extracted AML byte length: {len(aml_bytes)}")

# Construct ACPI DSDT Header (exact 36 bytes)
# uint32_t signature;       /* 0x00 */
# uint32_t length;          /* 0x04 */
# uint8_t  revision;        /* 0x08 */
# uint8_t  checksum;        /* 0x09 */
# char     oem_id[6];       /* 0x0A */
# char     oem_table_id[8]; /* 0x10 */
# uint32_t oem_revision;    /* 0x18 */
# uint32_t creator_id;      /* 0x1C */
# uint32_t creator_revision;/* 0x20 */

sig = b"DSDT"
length = 36 + len(aml_bytes)
revision = 2
checksum = 0
oem_id = b"ATOMS "
oem_table_id = b"ATOMSVM "
oem_revision = 1
creator_id = struct.unpack("<I", b"BOSM")[0]
creator_revision = 1

hdr = bytearray(struct.pack("<4sIBB6s8sIII",
    sig, length, revision, checksum, oem_id, oem_table_id,
    oem_revision, creator_id, creator_revision
))
assert len(hdr) == 36, f"Header size is {len(hdr)}, expected 36"

# compute checksum
full_table = bytearray(hdr + aml_bytes)
csum = (0x100 - (sum(full_table) & 0xFF)) & 0xFF
full_table[9] = csum
print(f"Computed DSDT checksum: 0x{csum:02X}, total length: {len(full_table)}")

dump_path = r"tools\verified_runtime_dsdt.aml"
with open(dump_path, "wb") as f:
    f.write(full_table)

# Run iasl -d on it
p = subprocess.run([r"tools\iasl\iasl.exe", "-d", dump_path], capture_output=True, text=True)
print("iasl stdout:")
print(p.stdout)
print("iasl stderr:")
print(p.stderr)

dsl_path = r"tools\verified_runtime_dsdt.dsl"
if os.path.exists(dsl_path):
    print("--- Disassembled DSL Output ---")
    with open(dsl_path, "r") as f:
        print(f.read())
