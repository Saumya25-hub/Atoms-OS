#!/usr/bin/env python3
"""
ATOMS OS — Userspace C/C++ Runtime Build Orchestrator
Builds libatoms_c.a (from upstream musl + ATOMS adapters)
and libatoms_cpp.a (from upstream LLVM libc++abi).
"""

import os
import sys
import subprocess
import glob
import re

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", ".."))
RUNTIME_DIR = os.path.join(ROOT, "atoms", "userspace", "runtime")
MUSL_DIR = os.path.join(ROOT, "third_party", "musl")
LLVM_DIR = os.path.join(ROOT, "third_party", "llvm")
BUILD_DIR = os.path.join(RUNTIME_DIR, "build")

CLANG = "clang"
CLANGXX = "clang++"
LLVM_AR = "llvm-ar"
import shutil
LLD = shutil.which("ld.lld") or shutil.which("lld") or r"C:\Program Files\LLVM\bin\ld.lld.exe"

COMMON_CFLAGS = [
    "-target", "x86_64-unknown-none-elf",
    "-nostdinc",
    "-ffreestanding",
    "-fno-pie",
    "-fno-pic",
    "-mcmodel=small",
    "-mno-red-zone",
    "-D_GNU_SOURCE",
    "-O2",
    "-g0",
    f"-I{ROOT}",
]

INCLUDES_C = [
    f"-I{RUNTIME_DIR}/include",
    f"-I{MUSL_DIR}/src/include",
    f"-I{MUSL_DIR}/arch/x86_64",
    f"-I{MUSL_DIR}/arch/generic",
    f"-I{MUSL_DIR}/src/internal",
    f"-I{MUSL_DIR}/include",
]

INCLUDES_CPP = [
    f"-I{RUNTIME_DIR}/include",
    f"-I{LLVM_DIR}/libcxxabi/include",
    f"-I{LLVM_DIR}/libcxx/include",
    f"-I{LLVM_DIR}/libcxx/src",
    f"-I{MUSL_DIR}/arch/x86_64",
    f"-I{MUSL_DIR}/arch/generic",
    f"-I{MUSL_DIR}/include",
]

CPPFLAGS = COMMON_CFLAGS + [
    "-std=c++23",
    "-nostdinc++",
    "-fno-exceptions",
    "-fno-rtti",
    "-D_GNU_SOURCE",
    "-D_LIBCPP_BUILDING_LIBRARY",
    "-D_LIBCPP_PSTL_BACKEND_SERIAL",
] + INCLUDES_CPP

CFLAGS = COMMON_CFLAGS + INCLUDES_C

def run_cmd(cmd, desc=""):
    res = subprocess.run(cmd, capture_output=True, text=True, shell=False)
    if res.returncode != 0:
        print(f"FAILED: {desc}\nCommand: {' '.join(cmd)}")
        print("STDOUT:", res.stdout)
        print("STDERR:", res.stderr)
        sys.exit(res.returncode)
    return res

def ensure_headers():
    print(">> Ensuring generated headers...")
    bits_dir = os.path.join(RUNTIME_DIR, "include", "bits")
    os.makedirs(bits_dir, exist_ok=True)

    # 1. Generate bits/alltypes.h
    alltypes_h = os.path.join(bits_dir, "alltypes.h")
    lines = []
    in_files = [
        os.path.join(MUSL_DIR, "arch", "x86_64", "bits", "alltypes.h.in"),
        os.path.join(MUSL_DIR, "include", "alltypes.h.in")
    ]
    for f in in_files:
        with open(f, 'r') as fp:
            for line in fp:
                m = re.match(r'^TYPEDEF (.*) ([^ ]*);$', line.strip())
                if m:
                    lines.append(f'#if defined(__NEED_{m.group(2)}) && !defined(__DEFINED_{m.group(2)})\ntypedef {m.group(1)} {m.group(2)};\n#define __DEFINED_{m.group(2)}\n#endif\n')
                    continue
                m = re.match(r'^STRUCT\s+([^\s]+)\s+(.*);$', line.strip())
                if m:
                    lines.append(f'#if defined(__NEED_struct_{m.group(1)}) && !defined(__DEFINED_struct_{m.group(1)})\nstruct {m.group(1)} {m.group(2)};\n#define __DEFINED_struct_{m.group(1)}\n#endif\n')
                    continue
                m = re.match(r'^UNION\s+([^\s]+)\s+(.*);$', line.strip())
                if m:
                    lines.append(f'#if defined(__NEED_union_{m.group(1)}) && !defined(__DEFINED_union_{m.group(1)})\nunion {m.group(1)} {m.group(2)};\n#define __DEFINED_union_{m.group(1)}\n#endif\n')
                    continue
                lines.append(line)
    with open(alltypes_h, 'w') as fp:
        fp.write(''.join(lines))

    # 2. Copy arch bits/*.h
    for f in glob.glob(os.path.join(MUSL_DIR, "arch", "x86_64", "bits", "*.h")):
        fname = os.path.basename(f)
        with open(f, 'rb') as s, open(os.path.join(bits_dir, fname), 'wb') as d:
            d.write(s.read())

    # 3. Generate bits/syscall.h
    syscall_h = os.path.join(bits_dir, "syscall.h")
    with open(os.path.join(MUSL_DIR, "arch", "x86_64", "bits", "syscall.h.in"), 'r') as fp:
        content = fp.read()
    sys_lines = []
    for line in content.splitlines():
        if line.startswith('#define __NR_'):
            sys_lines.append(line.replace('#define __NR_', '#define SYS_'))
    out = content + '\n' + '\n'.join(sys_lines) + '\n'
    with open(syscall_h, 'w') as fp:
        fp.write(out)

    print(">> Headers verified successfully.")

def build_libc():
    print(">> Building libatoms_c.a (musl libc + ATOMS adapters)...")
    c_build_dir = os.path.join(BUILD_DIR, "libc")
    os.makedirs(c_build_dir, exist_ok=True)

    musl_sources = [
        # String
        "src/string/memcpy.c",
        "src/string/memset.c",
        "src/string/memmove.c",
        "src/string/memcmp.c",
        "src/string/memchr.c",
        "src/string/memrchr.c",
        "src/string/strlen.c",
        "src/string/strcpy.c",
        "src/string/strncpy.c",
        "src/string/stpncpy.c",
        "src/string/strcmp.c",
        "src/string/strncmp.c",
        "src/string/strchr.c",
        "src/string/strrchr.c",
        "src/string/strcat.c",
        "src/string/strncat.c",
        "src/string/strstr.c",
        "src/string/strdup.c",
        "src/string/strspn.c",
        "src/string/strcspn.c",
        "src/string/strpbrk.c",
        "src/string/strtok_r.c",
        "src/string/strchrnul.c",
        "src/string/strnlen.c",
        "src/string/wcschr.c",
        "src/string/wcslen.c",
        # Stdio
        "src/stdio/snprintf.c",
        "src/stdio/vsnprintf.c",
        "src/stdio/sprintf.c",
        "src/stdio/vsprintf.c",
        "src/stdio/vfprintf.c",
        "src/stdio/__towrite.c",
        "src/stdio/__stdio_exit.c",
        "src/stdio/ofl.c",
        "src/stdio/fwrite.c",
        "src/stdio/fopen.c",
        "src/stdio/fclose.c",
        "src/stdio/fflush.c",
        "src/stdio/__fdopen.c",
        "src/stdio/__fmodeflags.c",
        "src/stdio/__stdio_write.c",
        "src/stdio/__stdio_read.c",
        "src/stdio/__stdio_close.c",
        "src/stdio/__stdout_write.c",
        "src/stdio/__toread.c",
        "src/stdio/__uflow.c",
        "src/stdio/__overflow.c",
        "src/stdio/ofl_add.c",
        "src/stdio/fprintf.c",
        "src/stdio/vfwprintf.c",
        "src/stdio/vfwscanf.c",
        "src/stdio/sscanf.c",
        "src/stdio/swprintf.c",
        "src/stdio/vasprintf.c",
        "src/stdio/vsscanf.c",
        "src/stdio/vfscanf.c",
        "src/stdio/vswprintf.c",
        "src/stdio/vswscanf.c",
        "src/stdio/stderr.c",
        "src/stdio/stdin.c",
        "src/stdio/stdout.c",
        "src/stdio/fwide.c",
        "src/stdio/fputwc.c",
        "src/stdio/getwc.c",
        "src/stdio/ungetwc.c",
        "src/stdio/fgetwc.c",
        "src/stdio/ungetc.c",
        "src/stdio/fscanf.c",
        # Internal / Shgetc
        "src/internal/shgetc.c",
        "src/internal/intscan.c",
        "src/internal/floatscan.c",
        "src/internal/syscall_ret.c",
        # Time
        "src/time/gettimeofday.c",
        "src/time/localtime_r.c",
        "src/time/strftime.c",
        "src/time/__secs_to_tm.c",
        "src/time/__tm_to_secs.c",
        "src/time/__month_to_secs.c",
        "src/time/__year_to_secs.c",
        # Unistd
        "src/unistd/unlink.c",
        # Multibyte
        "src/multibyte/wctomb.c",
        "src/multibyte/wcrtomb.c",
        "src/multibyte/btowc.c",
        "src/multibyte/mbrlen.c",
        "src/multibyte/mbrtowc.c",
        "src/multibyte/mbsnrtowcs.c",
        "src/multibyte/mbsrtowcs.c",
        "src/multibyte/mbtowc.c",
        "src/multibyte/wctob.c",
        "src/multibyte/wcsnrtombs.c",
        "src/multibyte/internal.c",
        "src/multibyte/mbsinit.c",
        "src/multibyte/wcsrtombs.c",
        # Math helpers
        "src/math/__fpclassifyl.c",
        "src/math/__signbitl.c",
        "src/math/frexpl.c",
        "src/math/scalbn.c",
        "src/math/ceil.c",
        "src/math/copysignl.c",
        "src/math/fabsl.c",
        "src/math/fmodl.c",
        "src/math/scalbnl.c",
        # Stdlib
        "src/stdlib/strtol.c",
        "src/stdlib/atoi.c",
        "src/stdlib/atol.c",
        "src/stdlib/atoll.c",
        "src/stdlib/qsort.c",
        "src/stdlib/bsearch.c",
        "src/stdlib/abs.c",
        "src/stdlib/labs.c",
        "src/stdlib/strtod.c",
        "src/stdlib/wcstod.c",
        "src/stdlib/wcstol.c",
        # PRNG
        "src/prng/rand.c",
        "src/prng/__seed48.c",
        "src/prng/__rand48_step.c",
        # Ctype & Locale
        "src/ctype/isalpha.c",
        "src/ctype/isdigit.c",
        "src/ctype/isalnum.c",
        "src/ctype/isspace.c",
        "src/ctype/isxdigit.c",
        "src/ctype/toupper.c",
        "src/ctype/tolower.c",
        "src/ctype/iswalpha.c",
        "src/ctype/iswblank.c",
        "src/ctype/iswcntrl.c",
        "src/ctype/iswdigit.c",
        "src/ctype/iswlower.c",
        "src/ctype/iswprint.c",
        "src/ctype/iswpunct.c",
        "src/ctype/iswspace.c",
        "src/ctype/iswupper.c",
        "src/ctype/iswxdigit.c",
        "src/ctype/towctrans.c",
        "src/ctype/isblank.c",
        "src/ctype/__ctype_get_mb_cur_max.c",
        "src/locale/freelocale.c",
        "src/locale/newlocale.c",
        "src/locale/uselocale.c",
        "src/locale/setlocale.c",
        "src/locale/localeconv.c",
        "src/locale/strcoll.c",
        "src/locale/strxfrm.c",
        "src/locale/wcscoll.c",
        "src/locale/wcsxfrm.c",
        "src/locale/strtod_l.c",
        "src/locale/c_locale.c",
        "src/locale/langinfo.c",
        # Strings & WCS
        "src/string/strncasecmp.c",
        "src/string/wmemchr.c",
        "src/string/wmemcmp.c",
        "src/string/wcscmp.c",
        "src/string/wmemcpy.c",
        "src/string/stpcpy.c",
        "src/string/wcsnlen.c",
        "src/string/strerror_r.c",
        # Exit & Assert
        "src/exit/assert.c",
        # Errno
        "src/errno/strerror.c",
    ]

    atoms_sources = [
        os.path.join(RUNTIME_DIR, "syscall", "atoms_syscall_adapter.c"),
        os.path.join(RUNTIME_DIR, "adapters", "atoms_heap.c"),
        os.path.join(RUNTIME_DIR, "pthread", "atoms_pthread.c"),
        os.path.join(RUNTIME_DIR, "tls", "atoms_tls.c"),
    ]

    objects = []

    # Compile musl sources
    for src in musl_sources:
        full_src = os.path.join(MUSL_DIR, src)
        obj = os.path.join(c_build_dir, f"musl_{os.path.basename(src).replace('.c', '.o')}")
        run_cmd([CLANG] + CFLAGS + ["-c", full_src, "-o", obj], f"Compile musl {os.path.basename(src)}")
        objects.append(obj)

    # Compile ATOMS adapters
    for src in atoms_sources:
        obj = os.path.join(c_build_dir, f"atoms_{os.path.basename(src).replace('.c', '.o')}")
        run_cmd([CLANG] + CFLAGS + ["-c", src, "-o", obj], f"Compile ATOMS {os.path.basename(src)}")
        objects.append(obj)

    # Compile standalone crt0.o
    crt0_src = os.path.join(RUNTIME_DIR, "startup", "crt0.c")
    crt0_obj = os.path.join(RUNTIME_DIR, "crt0.o")
    run_cmd([CLANG] + CFLAGS + ["-c", crt0_src, "-o", crt0_obj], "Compile crt0.o")
    print(f"   [OK] {crt0_obj}")

    # Archive libatoms_c.a
    lib_c = os.path.join(RUNTIME_DIR, "libatoms_c.a")
    if os.path.exists(lib_c):
        os.remove(lib_c)
    rsp_c = os.path.join(c_build_dir, "lib_c.rsp")
    with open(rsp_c, "w") as fp:
        fp.write("\n".join(objects))
    run_cmd([LLVM_AR, "rcs", lib_c, f"@{rsp_c}"], "Archive libatoms_c.a")
    print(f"   [OK] {lib_c} ({len(objects)} object modules)")

def build_libcpp():
    print(">> Building libatoms_cpp.a (LLVM libc++abi)...")
    cpp_build_dir = os.path.join(BUILD_DIR, "libcpp")
    os.makedirs(cpp_build_dir, exist_ok=True)

    cpp_sources = [
        os.path.join(LLVM_DIR, "libcxxabi/src/stdlib_new_delete.cpp"),
        os.path.join(LLVM_DIR, "libcxxabi/src/cxa_guard.cpp"),
        os.path.join(LLVM_DIR, "libcxxabi/src/cxa_virtual.cpp"),
        os.path.join(LLVM_DIR, "libcxxabi/src/stdlib_typeinfo.cpp"),
        os.path.join(LLVM_DIR, "libcxxabi/src/stdlib_exception.cpp"),
        os.path.join(RUNTIME_DIR, "adapters", "atoms_cpp_glue.cpp"),
        os.path.join(LLVM_DIR, "libcxx/src/string.cpp"),
        os.path.join(LLVM_DIR, "libcxx/src/verbose_abort.cpp"),
        os.path.join(LLVM_DIR, "libcxx/src/ios.cpp"),
        os.path.join(LLVM_DIR, "libcxx/src/ostream.cpp"),
        os.path.join(LLVM_DIR, "libcxx/src/iostream.cpp"),
        os.path.join(LLVM_DIR, "libcxx/src/locale.cpp"),
        os.path.join(LLVM_DIR, "libcxx/src/system_error.cpp"),
        os.path.join(LLVM_DIR, "libcxx/src/error_category.cpp"),
        os.path.join(LLVM_DIR, "libcxx/src/call_once.cpp"),
        os.path.join(LLVM_DIR, "libcxx/src/ios.instantiations.cpp"),
        os.path.join(LLVM_DIR, "libcxx/src/stdexcept.cpp"),
        os.path.join(LLVM_DIR, "libcxx/src/memory.cpp"),
        os.path.join(LLVM_DIR, "libcxx/src/functional.cpp"),
    ]

    objects = []
    for full_src in cpp_sources:
        obj = os.path.join(cpp_build_dir, f"cpp_{os.path.basename(full_src).replace('.cpp', '.o')}")
        run_cmd([CLANGXX] + CPPFLAGS + ["-c", full_src, "-o", obj], f"Compile {os.path.basename(full_src)}")
        objects.append(obj)

    lib_cpp = os.path.join(RUNTIME_DIR, "libatoms_cpp.a")
    if os.path.exists(lib_cpp):
        os.remove(lib_cpp)
    rsp_cpp = os.path.join(cpp_build_dir, "lib_cpp.rsp")
    with open(rsp_cpp, "w") as fp:
        fp.write("\n".join(objects))
    run_cmd([LLVM_AR, "rcs", lib_cpp, f"@{rsp_cpp}"], "Archive libatoms_cpp.a")
    print(f"   [OK] {lib_cpp} ({len(objects)} object modules)")

def build_tests():
    print(">> Building and linking test binaries in atoms/userspace/tests/...")
    tests_dir = os.path.join(ROOT, "atoms", "userspace", "tests")
    linker_script = os.path.join(ROOT, "userspace", "linker.ld")
    crt0_obj = os.path.join(RUNTIME_DIR, "crt0.o")
    lib_c = os.path.join(RUNTIME_DIR, "libatoms_c.a")
    lib_cpp = os.path.join(RUNTIME_DIR, "libatoms_cpp.a")

    # 1. test_c_basic
    c_src = os.path.join(tests_dir, "test_c_basic.c")
    c_obj = os.path.join(tests_dir, "test_c_basic.o")
    c_elf = os.path.join(tests_dir, "test_c_basic.elf")
    run_cmd([CLANG] + CFLAGS + ["-c", c_src, "-o", c_obj], "Compile test_c_basic.c")
    run_cmd([LLD, "-T", linker_script, crt0_obj, c_obj, lib_c, "-o", c_elf], "Link test_c_basic.elf")
    print(f"   [OK] {c_elf}")

    # 2. test_cpp_basic
    cpp_src = os.path.join(tests_dir, "test_cpp_basic.cpp")
    cpp_obj = os.path.join(tests_dir, "test_cpp_basic.o")
    cpp_elf = os.path.join(tests_dir, "test_cpp_basic.elf")
    run_cmd([CLANGXX] + CPPFLAGS + ["-c", cpp_src, "-o", cpp_obj], "Compile test_cpp_basic.cpp")
    run_cmd([LLD, "-T", linker_script, crt0_obj, cpp_obj, lib_cpp, lib_c, "-o", cpp_elf], "Link test_cpp_basic.elf")
    print(f"   [OK] {cpp_elf}")

    # 3. test_multithread_sync
    mt_src = os.path.join(tests_dir, "test_multithread_sync.cpp")
    mt_obj = os.path.join(tests_dir, "test_multithread_sync.o")
    mt_elf = os.path.join(tests_dir, "test_multithread_sync.elf")
    run_cmd([CLANGXX] + CPPFLAGS + ["-c", mt_src, "-o", mt_obj], "Compile test_multithread_sync.cpp")
    run_cmd([LLD, "-T", linker_script, crt0_obj, mt_obj, lib_cpp, lib_c, "-o", mt_elf], "Link test_multithread_sync.elf")
    print(f"   [OK] {mt_elf}")

def main():
    print("==================================================")
    print("  ATOMS OS — Userspace C/C++ Runtime Builder      ")
    print("==================================================")
    ensure_headers()
    build_libc()
    build_libcpp()
    build_tests()
    print(">> SUCCESS: All Userspace Runtimes and Test Binaries Built & Linked Cleanly!")

if __name__ == "__main__":
    main()
