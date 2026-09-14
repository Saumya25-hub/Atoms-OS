#!/usr/bin/env python3
"""
ATOMS OS — Audio Test Corpus Generator
Generates real audio files across WAV, MP3, FLAC, AAC, and Vorbis
using clean synthetic acoustic waveforms (sine wave chords & sweeps).
"""

import os
import subprocess

OUT_DIR = "test_audio"
os.makedirs(OUT_DIR, exist_ok=True)

# Generate base 44.1kHz and 48kHz WAV audio files using ffmpeg synth
corpus = [
    # WAV Matrix
    ("wav_mono_44k_16bit.wav", "-f lavfi -i sine=frequency=440:sample_rate=44100:duration=3 -ac 1 -c:a pcm_s16le"),
    ("wav_stereo_44k_16bit.wav", "-f lavfi -i sine=frequency=523:sample_rate=44100:duration=3 -ac 2 -c:a pcm_s16le"),
    ("wav_stereo_48k_16bit.wav", "-f lavfi -i sine=frequency=587:sample_rate=48000:duration=3 -ac 2 -c:a pcm_s16le"),
    ("wav_stereo_96k_24bit.wav", "-f lavfi -i sine=frequency=659:sample_rate=96000:duration=3 -ac 2 -c:a pcm_s24le"),

    # MP3 Matrix
    ("mp3_cbr_128k_44k.mp3", "-f lavfi -i sine=frequency=440:sample_rate=44100:duration=3 -ac 2 -b:a 128k -c:a libmp3lame -metadata title=\"Test CBR 128k\" -metadata artist=\"ATOMS Audio Engine\""),
    ("mp3_cbr_320k_48k.mp3", "-f lavfi -i sine=frequency=523:sample_rate=48000:duration=3 -ac 2 -b:a 320k -c:a libmp3lame -metadata title=\"Test CBR 320k\" -metadata artist=\"ATOMS Audio Engine\""),
    ("mp3_vbr_44k.mp3", "-f lavfi -i sine=frequency=659:sample_rate=44100:duration=3 -ac 2 -q:a 2 -c:a libmp3lame -metadata title=\"Test VBR Q2\" -metadata artist=\"ATOMS Audio Engine\""),
    ("mp3_mono_128k_44k.mp3", "-f lavfi -i sine=frequency=440:sample_rate=44100:duration=3 -ac 1 -b:a 128k -c:a libmp3lame"),

    # FLAC Matrix
    ("flac_16bit_44k.flac", "-f lavfi -i sine=frequency=440:sample_rate=44100:duration=3 -ac 2 -c:a flac -metadata title=\"Test FLAC 16-bit\" -metadata artist=\"ATOMS Audio Engine\""),
    ("flac_24bit_48k.flac", "-f lavfi -i sine=frequency=523:sample_rate=48000:duration=3 -ac 2 -c:a flac -sample_fmt s32 -bits_per_raw_sample 24"),
    ("flac_24bit_96k.flac", "-f lavfi -i sine=frequency=659:sample_rate=96000:duration=3 -ac 2 -c:a flac -sample_fmt s32 -bits_per_raw_sample 24"),

    # AAC Matrix (ADTS containers)
    ("aac_lc_44k.aac", "-f lavfi -i sine=frequency=440:sample_rate=44100:duration=3 -ac 2 -c:a aac -b:a 160k"),
    ("aac_lc_48k.aac", "-f lavfi -i sine=frequency=523:sample_rate=48000:duration=3 -ac 2 -c:a aac -b:a 192k"),

    # Ogg Vorbis Matrix
    ("vorbis_44k.ogg", "-f lavfi -i sine=frequency=440:sample_rate=44100:duration=3 -ac 2 -c:a libvorbis -q:a 4 -metadata title=\"Test Vorbis\" -metadata artist=\"ATOMS Audio Engine\""),
    ("vorbis_48k.ogg", "-f lavfi -i sine=frequency=523:sample_rate=48000:duration=3 -ac 2 -c:a libvorbis -q:a 5")
]

print("[CORPUS] Generating test audio corpus via ffmpeg...")
for fname, args in corpus:
    out_path = os.path.join(OUT_DIR, fname)
    cmd = f"ffmpeg -y {args} \"{out_path}\""
    res = subprocess.run(cmd, shell=True, capture_output=True)
    if res.returncode == 0:
        sz = os.path.getsize(out_path)
        print(f"  [OK] Generated {fname:<25} ({sz:>7} bytes)")
    else:
        print(f"  [FAIL] Failed to generate {fname}: {res.stderr.decode('utf-8', errors='ignore')[:100]}")

print("\n[CORPUS] Generation complete!")
