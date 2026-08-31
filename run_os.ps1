# Launch SignaturesOS in QEMU Emulator
Write-Host "=========================================" -ForegroundColor Cyan
Write-Host "      Launching SignaturesOS (QEMU)      " -ForegroundColor Cyan
Write-Host "=========================================" -ForegroundColor Cyan

$qemu = "qemu-system-x86_64"
if (-not (Get-Command $qemu -ErrorAction SilentlyContinue)) {
    if (Test-Path "C:\Program Files\qemu\qemu-system-x86_64.exe") {
        $qemu = "C:\Program Files\qemu\qemu-system-x86_64.exe"
    }
}

$bios_arg = @()
if (Test-Path "C:\Program Files\qemu\share\edk2-x86_64-code.fd") {
    $bios_arg = @("-bios", "C:\Program Files\qemu\share\edk2-x86_64-code.fd")
}

& $qemu $bios_arg -cpu qemu64 -smp 2 -m 4096 -vga std -device qemu-xhci,id=xhci0 -device usb-mouse,bus=xhci0.0 -netdev user,id=net0 -device e1000,netdev=net0 -drive file="build\OS.img",format=raw,index=0,media=disk -serial file:kernel_log.txt
