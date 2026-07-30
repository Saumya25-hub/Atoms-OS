# Launch SignaturesOS in QEMU Emulator
Write-Host "=========================================" -ForegroundColor Cyan
Write-Host "      Launching SignaturesOS (QEMU)      " -ForegroundColor Cyan
Write-Host "=========================================" -ForegroundColor Cyan

qemu-system-x86_64 -accel whpx -cpu qemu64 -smp 1 -m 2048 -vga std -device qemu-xhci,id=xhci0 -device usb-mouse,bus=xhci0.0 -netdev user,id=net0 -device e1000,netdev=net0 -audiodev dsound,id=audio0 -device AC97,audiodev=audio0 -drive file="build\OS.img",format=raw,index=0,media=disk -serial file:kernel_log.txt
