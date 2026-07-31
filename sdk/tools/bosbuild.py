#!/usr/bin/env python3
"""
BOS OS SDK Build Tool (bosbuild)
Compiles C and C++ applications using the BOS Developer SDK headers and libraries.
"""

import sys
import os
import subprocess

def main():
    print("=========================================")
    print("       BOS SDK Application Builder       ")
    print("=========================================")

    if len(sys.argv) < 2:
        print("Usage: python bosbuild.py <source_file.c/cpp> [output_file.elf]")
        sys.exit(1)

    src_file = sys.argv[1]
    out_file = sys.argv[2] if len(sys.argv) > 2 else "app.elf"

    is_cpp = src_file.endswith(".cpp") or src_file.endswith(".cc")
    compiler = "clang++" if is_cpp else "clang"

    cmd = [
        compiler,
        "-target", "x86_64-pc-none-elf",
        "-mno-sse", "-mno-sse2", "-mno-mmx", "-msoft-float",
        "-ffreestanding", "-mno-red-zone",
        "-I.", "-I", "sdk/include", "-I", "platform/include", "-I", "framework/include",
        "-c", src_file,
        "-o", out_file
    ]

    print(f"[SDK BUILD] Executing: {' '.join(cmd)}")
    res = subprocess.run(cmd)
    if res.returncode == 0:
        print(f"[SDK BUILD SUCCESS] Output binary: {out_file}")
    else:
        print(f"[SDK BUILD ERROR] Compilation failed with code {res.returncode}")
        sys.exit(res.returncode)

if __name__ == "__main__":
    main()
