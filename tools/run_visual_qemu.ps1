# =============================================================================
# ATOMS OS — QEMU Visual Display Runner (UEFI + Hypervisor + FreeBSD)
# =============================================================================

$qemuExe = "D:\OS-QEMU-EMU\qemu\qemu-system-x86_64.exe"
if (-not (Test-Path $qemuExe)) {
    $qemuExe = "C:\Program Files\qemu\qemu-system-x86_64.exe"
}

$ovmf = "D:\OS-QEMU-EMU\qemu\share\edk2-x86_64-code.fd"
if (-not (Test-Path $ovmf)) {
    $ovmf = "C:\Program Files\qemu\share\edk2-x86_64-code.fd"
}

$img = "build\atoms_uefi_test.img"
if (-not (Test-Path $img)) {
    Write-Host "[ERROR] $img not found! Run build.ps1 first." -ForegroundColor Red
    exit 1
}

Write-Host "==========================================================" -ForegroundColor Cyan
Write-Host "  Launching ATOMS OS in QEMU (Visual Display + UEFI Mode) " -ForegroundColor Cyan
Write-Host "==========================================================" -ForegroundColor Cyan
Write-Host "  QEMU Binary : $qemuExe" -ForegroundColor Yellow
Write-Host "  OVMF BIOS   : $ovmf" -ForegroundColor Yellow
Write-Host "  Disk Image  : $img" -ForegroundColor Yellow
Write-Host "  Serial Log  : Streaming to this console (COM1/stdio)" -ForegroundColor Yellow
Write-Host "==========================================================" -ForegroundColor Cyan

& $qemuExe `
    -machine q35 `
    -cpu max,vmx=on `
    -m 4096M `
    -drive if=pflash,format=raw,readonly=on,file=$ovmf `
    -drive file=$img,format=raw `
    -device qemu-xhci `
    -device usb-kbd `
    -device usb-mouse `
    -netdev user,id=net0 `
    -device e1000,netdev=net0 `
    -serial stdio `
    -vga std
