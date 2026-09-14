#!/usr/bin/env python3
"""
ATOMS OS — Phase M2: Universal Audio Engine & Modern Codec Playback Verification
Full Automated Forensic Test Suite:
  1. Host Audio Codec & Fixed-Point Resampler Forensic Telemetry (15 files)
  2. Pure UEFI QEMU Boot: Intel HDA Controller + Realtek Codec + Software Mixer + Codec Registry
  3. Pure UEFI QEMU Boot: AC97 Legacy Hardware Regression
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
HDA_LOG = r"build\m2_audio_hda_serial.log"
AC97_LOG = r"build\m2_audio_ac97_serial.log"
HOST_HARNESS = r"tools\test_audio_codecs_host.exe"

def run_host_codec_harness():
    print("========================================================================================")
    print("  STAGE 1: HOST AUDIO CODEC & RESAMPLER FORENSIC TELEMETRY VERIFICATION")
    print("========================================================================================")
    if not os.path.exists(HOST_HARNESS):
        print(f"[ERROR] Host test harness not found at {HOST_HARNESS}")
        return False

    proc = subprocess.run([HOST_HARNESS], capture_output=True, text=True)
    print(proc.stdout)
    if proc.stderr:
        print(proc.stderr)

    return proc.returncode == 0

def run_qemu_test(name, device_args, log_file, expected_marks):
    print(f"\n========================================================================================")
    print(f"  STAGE 2: QEMU PURE UEFI BOOT — {name.upper()}")
    print(f"========================================================================================")

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

    print(f"[QEMU] Waiting for kernel boot and audio engine telemetry...")
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

    print(f"\n--- Serial Telemetry ({log_file}) ---")
    if os.path.exists(log_file):
        with open(log_file, "r", encoding="utf-8", errors="ignore") as f:
            lines = f.readlines()

        for line in lines:
            if any(k in line for k in ["[BOS-AUDIO]", "[AUDIO]", "[AC97]", "[BOOT]", "READY", "Codec", "HDA"]):
                print("  " + line.strip())

        full_content = "".join(lines)
        results = {}
        for mark in expected_marks:
            found = mark in full_content
            results[mark] = found
            status = "PASS" if found else "FAIL"
            print(f"  [{status}] Mark: '{mark}'")

        success = all(results.values())
        print(f"\nSub-Test Verdict: {'PASS' if success else 'FAIL'}")
        return success
    else:
        print(f"[ERROR] Serial log file not found: {log_file}")
        return False

def main():
    print("########################################################################################")
    print("  ATOMS OS — PHASE M2: UNIVERSAL AUDIO ENGINE & CODECS MASTER CERTIFICATION SUITE")
    print("########################################################################################\n")

    # 1. Host Codecs + Resampler + Channel Matrix
    host_pass = run_host_codec_harness()

    # 2. Pure UEFI QEMU Intel HDA + Mixer + Codec Registry
    hda_marks = [
        "[BOS-AUDIO] Audio subsystem init",
        "[AUDIO] Universal Software Mixer Ready (Resampler + Multi-Rate Enabled)",
        "[AUDIO] Codec Registry Initialized (WAV, MP3, FLAC, AAC, Vorbis)",
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
    hda_pass = run_qemu_test(
        "Intel HDA Controller + Software Mixer + Codec Registry",
        ["-device", "intel-hda", "-device", "hda-duplex,audiodev=audio0"],
        HDA_LOG,
        hda_marks
    )

    # 3. Pure UEFI QEMU AC97 Legacy Hardware Regression
    ac97_marks = [
        "[BOS-AUDIO] Audio subsystem init",
        "[AUDIO] Universal Software Mixer Ready (Resampler + Multi-Rate Enabled)",
        "[AUDIO] Codec Registry Initialized (WAV, MP3, FLAC, AAC, Vorbis)",
        "[BOS-AUDIO] PCI audio devices scanning",
        "Codec Ready: PASS",
        "[BOS-AUDIO] Active audio driver: Intel AC97 Audio Controller"
    ]
    ac97_pass = run_qemu_test(
        "AC97 Legacy Hardware Fallback (Regression Test)",
        ["-device", "AC97,audiodev=audio0"],
        AC97_LOG,
        ac97_marks
    )

    print("\n========================================================================================")
    print("  PHASE M2 MASTER CERTIFICATION SUMMARY")
    print("========================================================================================")
    print(f"  1. Host Codec & Resampler Telemetry: {'PASS' if host_pass else 'FAIL'}")
    print(f"  2. Intel HDA + Mixer + Registry:      {'PASS' if hda_pass else 'FAIL'}")
    print(f"  3. AC97 Legacy Fallback:             {'PASS' if ac97_pass else 'FAIL'}")

    all_passed = host_pass and hda_pass and ac97_pass
    print(f"\nOVERALL PHASE M2 CERTIFICATION VERDICT: {'PASS (100% SUCCESS)' if all_passed else 'FAIL'}")
    print("========================================================================================")

    if all_passed:
        sys.exit(0)
    else:
        sys.exit(1)

if __name__ == "__main__":
    main()
