#!/usr/bin/env python3
"""
ATOMS OS — M1 Universal Audio Hardware Foundation Verification
Tests pure UEFI boot with OVMF + Intel HDA (intel-hda + hda-duplex)
and verifies AC97 regression compatibility.
"""

import os
import sys
import time
import subprocess
import shutil

QEMU_EXE = r"D:\OS-QEMU-EMU\qemu\qemu-system-x86_64.exe"
if not os.path.exists(QEMU_EXE):
    QEMU_EXE = shutil.which("qemu-system-x86_64") or "qemu-system-x86_64"

OVMF_BIOS = r"D:\OS-QEMU-EMU\qemu\share\edk2-x86_64-code.fd"
GPT_IMG = r"build\atoms_uefi_test.img"
HDA_LOG = r"build\m1_audio_hda_serial.log"
AC97_LOG = r"build\m1_audio_ac97_serial.log"

def run_test(name, device_args, log_file, expected_marks):
    print(f"\n========================================================")
    print(f"  RUNNING TEST: {name}")
    print(f"========================================================")

    if os.path.exists(log_file):
        try:
            os.remove(log_file)
        except OSError:
            pass

    cmd = [
        QEMU_EXE,
        "-drive", f"if=pflash,format=raw,readonly=on,file={OVMF_BIOS}",
        "-drive", f"file={GPT_IMG},format=raw",
        "-device", "qemu-xhci",
        "-device", "usb-mouse",
        "-device", "usb-kbd",
        "-audiodev", "none,id=audio0",
    ] + device_args + [
        "-serial", f"file:{log_file}",
        "-m", "2048M",
        "-display", "none",
        "-no-reboot"
    ]

    print(f"[QEMU] Command: {' '.join(cmd)}")
    proc = subprocess.Popen(cmd)

    # Wait up to 30 seconds for kernel initialization
    print(f"[QEMU] Waiting for kernel boot and audio initialization...")
    start_time = time.time()
    passed = False
    
    for _ in range(30):
        time.sleep(1)
        if os.path.exists(log_file):
            try:
                with open(log_file, "r", encoding="utf-8", errors="ignore") as f:
                    content = f.read()
                all_found = True
                for mark in expected_marks:
                    if mark not in content:
                        all_found = False
                        break
                if all_found:
                    passed = True
                    print(f"[QEMU] All required marks detected after {int(time.time() - start_time)}s!")
                    break
            except Exception:
                pass
        if proc.poll() is not None:
            break

    try:
        proc.terminate()
        proc.wait(timeout=3)
    except Exception:
        try:
            proc.kill()
        except Exception:
            pass

    print(f"\n--- Serial Output ({log_file}) ---")
    if os.path.exists(log_file):
        with open(log_file, "r", encoding="utf-8", errors="ignore") as f:
            lines = f.readlines()
        
        # Print relevant lines
        for line in lines:
            if any(k in line for k in ["[BOS-AUDIO]", "[AUDIO]", "[AC97]", "[BOOT]", "READY", "Codec", "HDA"]):
                print("  " + line.strip())

        # Evaluate marks
        full_content = "".join(lines)
        results = {}
        for mark in expected_marks:
            found = mark in full_content
            results[mark] = found
            status = "PASS" if found else "FAIL"
            print(f"  [{status}] Expected mark: '{mark}'")

        success = all(results.values())
        print(f"\nTest Verdict: {'PASS' if success else 'FAIL'}")
        return success
    else:
        print(f"[ERROR] Serial log file not found: {log_file}")
        return False

def main():
    print("ATOMS OS Phase M1: Universal Audio Hardware Foundation Verification")
    
    # 1. Test Intel HDA Controller + Codec
    hda_marks = [
        "[BOS-AUDIO] Audio subsystem init",
        "[BOS-AUDIO] PCI audio devices scanning",
        "[BOS-AUDIO] Intel HDA controller detected",
        "BAR type: MMIO",
        "[BOS-AUDIO] Controller reset: OK",
        "[BOS-AUDIO] Codec scan",
        "[BOS-AUDIO] Codec backend:",
        "[BOS-AUDIO] Output path: DAC",
        "[BOS-AUDIO] PCM capability: 48000Hz 16-bit 2-channel Stereo",
        "[BOS-AUDIO] DMA stream initialized",
        "[BOS-AUDIO] Audio output READY",
        "[BOS-AUDIO] Active audio driver: Intel High Definition Audio (HDA)"
    ]
    hda_pass = run_test(
        "Intel HDA Controller & Codec Initialization",
        ["-device", "intel-hda", "-device", "hda-duplex,audiodev=audio0"],
        HDA_LOG,
        hda_marks
    )

    # 2. Regression Test: Legacy AC97
    ac97_marks = [
        "[BOS-AUDIO] Audio subsystem init",
        "[BOS-AUDIO] PCI audio devices scanning",
        "Codec Ready: PASS",
        "[BOS-AUDIO] Active audio driver: Intel AC97 Audio Controller"
    ]
    ac97_pass = run_test(
        "AC97 Legacy Audio Controller (Regression Test)",
        ["-device", "AC97,audiodev=audio0"],
        AC97_LOG,
        ac97_marks
    )

    print("\n========================================================")
    print("  PHASE M1 AUTOMATED VERIFICATION SUMMARY")
    print("========================================================")
    print(f"  Intel HDA + Codec: {'PASS' if hda_pass else 'FAIL'}")
    print(f"  AC97 Legacy:       {'PASS' if ac97_pass else 'FAIL'}")

    if hda_pass and ac97_pass:
        print("\nOVERALL VERDICT: ALL TESTS PASSED (100% SUCCESS)")
        sys.exit(0)
    else:
        print("\nOVERALL VERDICT: ONE OR MORE TESTS FAILED")
        sys.exit(1)

if __name__ == "__main__":
    main()
