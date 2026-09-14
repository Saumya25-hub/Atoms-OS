#!/usr/bin/env python3
"""
ATOMS Platform Adaptation Layer (APAL) Test Runner
Compiles and links apal_test_suite.elf for ATOMS userspace.
"""

import os
import subprocess
import sys

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_ROOT = os.path.abspath(os.path.join(SCRIPT_DIR, "..", "..", "..", ".."))

CLANG = "clang"
LD_LLD = "ld.lld"

APAL_DIR = os.path.join(PROJECT_ROOT, "atoms", "userspace", "apal")
RUNTIME_DIR = os.path.join(PROJECT_ROOT, "atoms", "userspace", "runtime")
MUSL_DIR = os.path.join(PROJECT_ROOT, "third_party", "musl")
BUILD_DIR = os.path.join(PROJECT_ROOT, "build")

SRC_FILE = os.path.join(SCRIPT_DIR, "apal_test_suite.c")
OBJ_FILE = os.path.join(BUILD_DIR, "apal_test_suite.o")
ELF_FILE = os.path.join(BUILD_DIR, "apal_test_suite.elf")

CFLAGS = [
    "-target", "x86_64-unknown-none-elf",
    "-c",
    "-Wall",
    "-Wextra",
    "-Wno-unused-parameter",
    "-O2",
    "-nostdinc",
    "-ffreestanding",
    "-fno-pie",
    "-fno-pic",
    "-mcmodel=small",
    "-mno-red-zone",
    "-D_GNU_SOURCE",
    f"-I{PROJECT_ROOT}",
    f"-I{APAL_DIR}",
    f"-I{os.path.join(APAL_DIR, 'include')}",
    f"-I{os.path.join(RUNTIME_DIR, 'include')}",
    f"-I{os.path.join(MUSL_DIR, 'include')}",
    f"-I{os.path.join(MUSL_DIR, 'arch', 'x86_64')}",
    f"-I{os.path.join(MUSL_DIR, 'arch', 'generic')}",
]

def run():
    os.makedirs(BUILD_DIR, exist_ok=True)

    # 1. Compile test suite source
    print(f"[TEST] Compiling {os.path.basename(SRC_FILE)}...")
    cmd_cc = [CLANG] + CFLAGS + [SRC_FILE, "-o", OBJ_FILE]
    res = subprocess.run(cmd_cc, shell=True, capture_output=True, text=True)
    if res.returncode != 0:
        print("[TEST] Compilation FAILED:")
        print("STDERR:", res.stderr)
        return 1

    # 2. Link against libapal.a, libatoms_cpp.a, libatoms_c.a, crt0.o
    print(f"[TEST] Linking {os.path.basename(ELF_FILE)}...")
    linker_script = os.path.join(PROJECT_ROOT, "userspace", "linker.ld")
    crt0_obj = os.path.join(RUNTIME_DIR, "crt0.o")
    libapal = os.path.join(APAL_DIR, "libapal.a")
    libcpp = os.path.join(RUNTIME_DIR, "libatoms_cpp.a")
    libc = os.path.join(RUNTIME_DIR, "libatoms_c.a")

    cmd_ld = [
        LD_LLD,
        "-T", linker_script,
        crt0_obj,
        OBJ_FILE,
        libapal,
        libcpp,
        libc,
        "-o", ELF_FILE
    ]
    res = subprocess.run(cmd_ld, shell=True, capture_output=True, text=True)
    if res.returncode != 0:
        print("[TEST] Linking FAILED:")
        print("STDERR:", res.stderr)
        return 1

    sz = os.path.getsize(ELF_FILE)
    print(f"[TEST] SUCCESS: {ELF_FILE} ({sz} bytes)")
    return 0

if __name__ == "__main__":
    sys.exit(run())
