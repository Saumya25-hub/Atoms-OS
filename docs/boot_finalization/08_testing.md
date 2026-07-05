# Testing Guide

## QEMU Boot Test

```powershell
qemu-system-x86_64 -drive file=build/OS.img,format=raw,index=0,media=disk -m 512M -vga std -audiodev dsound,id=audio0 -device AC97,audiodev=audio0
```

## Test Cases

### TC-01: Silent Boot
- Boot the OS.
- **Expected**: Desktop appears with no sound.
- **Pass Criteria**: Zero audio output during the entire boot sequence.

### TC-02: Start Menu → Music Player
- Click Start Button.
- Click "Music Player" in the Start Menu.
- **Expected**: ATOMS Music window opens and DEMO1.wav begins playing.
- **Pass Criteria**: Audio plays through AC97 without corruption.

### TC-03: Multiple App Launch
- Open Terminal, Settings, and Calculator from the Start Menu.
- **Expected**: All windows spawn independently without interfering with the audio subsystem.
- **Pass Criteria**: No crashes, no audio glitches.

### TC-04: Boot Log Verification
- Check serial/display output during boot.
- **Expected**: Only concise `[AUDIO]` messages appear. No forensic dumps.
