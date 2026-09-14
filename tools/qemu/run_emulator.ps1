param (
    [string]$Mode = "UEFI"
)

$QEMU_EXE = "D:\OS-QEMU-EMU\qemu\qemu-system-x86_64.exe"
$UEFI_FD = "D:\OS-QEMU-EMU\qemu\share\edk2-x86_64-code.fd"
$IMG_FILE = "build\OS.img"

if (-not (Test-Path $IMG_FILE)) {
    Write-Host "ERROR: build\OS.img not found! Building OS first..." -ForegroundColor Red
    powershell -ExecutionPolicy Bypass -File .\build.ps1
}

Write-Host "=============================================" -ForegroundColor Cyan
Write-Host " SignaturesOS QEMU Real-Hardware Emulator" -ForegroundColor Cyan
Write-Host " Mode: $Mode" -ForegroundColor Yellow
Write-Host "=============================================" -ForegroundColor Cyan

if ($Mode -eq "UEFI") {
    Write-Host "Launching QEMU in Pure UEFI Hardware Emulation Mode..." -ForegroundColor Green
    & $QEMU_EXE -m 2048M -vga std -drive if=pflash,format=raw,readonly=on,file=$UEFI_FD -drive file=$IMG_FILE,format=raw -net none -serial stdio
} else {
    Write-Host "Launching QEMU in Legacy BIOS CSM Emulation Mode..." -ForegroundColor Green
    & $QEMU_EXE -m 2048M -drive file=$IMG_FILE,format=raw -net none
}
