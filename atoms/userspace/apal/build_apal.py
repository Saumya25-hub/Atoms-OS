#!/usr/bin/env python3
"""
ATOMS Platform Adaptation Layer (APAL) Build Script
Compiles all APAL modules and archives them into libapal.a
"""

import os
import subprocess
import sys

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_ROOT = os.path.abspath(os.path.join(SCRIPT_DIR, "..", "..", ".."))

CLANG = "clang"
LLVM_AR = "llvm-ar"

CFLAGS = [
    "-target", "x86_64-unknown-none-elf",
    "-c",
    "-Wall",
    "-Wextra",
    "-Wno-unused-parameter",
    "-Wno-unused-function",
    "-O2",
    "-nostdinc",
    "-ffreestanding",
    "-fno-stack-protector",
    "-mno-red-zone",
    "-fno-pic",
    "-fno-pie",
    "-mcmodel=small",
    "-D_GNU_SOURCE",
    f"-I{PROJECT_ROOT}",
    f"-I{os.path.join(PROJECT_ROOT, 'atoms', 'userspace', 'apal')}",
    f"-I{os.path.join(PROJECT_ROOT, 'atoms', 'userspace', 'apal', 'include')}",
    f"-I{os.path.join(PROJECT_ROOT, 'atoms', 'userspace', 'runtime', 'include')}",
    f"-I{os.path.join(PROJECT_ROOT, 'third_party', 'musl', 'include')}",
    f"-I{os.path.join(PROJECT_ROOT, 'third_party', 'musl', 'arch', 'x86_64')}",
    f"-I{os.path.join(PROJECT_ROOT, 'third_party', 'musl', 'arch', 'generic')}",
]

SOURCES = [
    os.path.join(SCRIPT_DIR, "apal.c"),
    os.path.join(SCRIPT_DIR, "memory", "apal_memory.c"),
    os.path.join(SCRIPT_DIR, "threads", "apal_thread.c"),
    os.path.join(SCRIPT_DIR, "process", "apal_process.c"),
    os.path.join(SCRIPT_DIR, "ipc", "apal_ipc.c"),
    os.path.join(SCRIPT_DIR, "shared_memory", "apal_shm.c"),
    os.path.join(SCRIPT_DIR, "filesystem", "apal_fs.c"),
    os.path.join(SCRIPT_DIR, "sockets", "apal_socket.c"),
    os.path.join(SCRIPT_DIR, "time", "apal_time.c"),
    os.path.join(SCRIPT_DIR, "randomness", "apal_rand.c"),
    os.path.join(SCRIPT_DIR, "graphics", "apal_surface.c"),
    os.path.join(SCRIPT_DIR, "input", "apal_input.c"),
    os.path.join(SCRIPT_DIR, "audio", "apal_audio.c"),
    os.path.join(SCRIPT_DIR, "loader", "apal_loader.c"),
    os.path.join(SCRIPT_DIR, "exceptions", "apal_exception.c"),
]

BUILD_DIR = os.path.join(SCRIPT_DIR, "build")
OUTPUT_LIB = os.path.join(SCRIPT_DIR, "libapal.a")

def build():
    os.makedirs(BUILD_DIR, exist_ok=True)
    obj_files = []

    print("[APAL] Compiling APAL subsystem modules...")
    for src in SOURCES:
        rel = os.path.relpath(src, SCRIPT_DIR)
        obj_name = rel.replace(os.sep, "_").replace(".c", ".o")
        obj_path = os.path.join(BUILD_DIR, obj_name)

        cmd = [CLANG] + CFLAGS + [src, "-o", obj_path]
        print(f"  CC  {rel} -> {obj_name}")
        res = subprocess.run(cmd, shell=True, capture_output=True, text=True)
        if res.returncode != 0:
            print(f"[APAL] Compilation failed for {src}")
            print("STDOUT:", res.stdout)
            print("STDERR:", res.stderr)
            return 1
        obj_files.append(obj_path)

    print(f"[APAL] Creating static archive {os.path.basename(OUTPUT_LIB)}...")
    if os.path.exists(OUTPUT_LIB):
        os.remove(OUTPUT_LIB)
    ar_cmd = [LLVM_AR, "rcs", OUTPUT_LIB] + obj_files
    res = subprocess.run(ar_cmd, shell=True, capture_output=True, text=True)
    if res.returncode != 0:
        print("[APAL] Archive creation failed.")
        print("STDERR:", res.stderr)
        return 1

    print(f"[APAL] SUCCESS: Generated {OUTPUT_LIB} ({os.path.getsize(OUTPUT_LIB)} bytes)")
    return 0

if __name__ == "__main__":
    sys.exit(build())
