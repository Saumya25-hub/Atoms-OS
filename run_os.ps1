# Launch SignaturesOS in QEMU Emulator
Write-Host "=========================================" -ForegroundColor Cyan
Write-Host "      Launching SignaturesOS (QEMU)      " -ForegroundColor Cyan
Write-Host "=========================================" -ForegroundColor Cyan

qemu-system-x86_64 -m 1024 -vga std -usb -device usb-tablet -drive file="build\OS.img",format=raw,index=0,media=disk -audiodev dsound,id=audio0 -device AC97,audiodev=audio0
