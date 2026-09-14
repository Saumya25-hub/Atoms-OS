$serialLog = 'build\phase2_qemu_serial.log'
if (Test-Path $serialLog) { Remove-Item $serialLog -Force }

$qemuExe = 'D:\OS-QEMU-EMU\qemu\qemu-system-x86_64.exe'
$biosFd = 'D:\OS-QEMU-EMU\qemu\share\edk2-x86_64-code.fd'

$qemuArgs = "-drive if=pflash,format=raw,readonly=on,file=$biosFd -drive file=build\atoms_uefi_test.img,format=raw -device qemu-xhci -device usb-kbd -device usb-mouse -audiodev none,id=audio0 -device intel-hda -device hda-duplex,audiodev=audio0 -serial file:$serialLog -m 2048M -display none -no-reboot"

Write-Host "Starting QEMU UEFI Pre-Flight Test for Phase 2 Media Engine..." -ForegroundColor Cyan
$proc = Start-Process -FilePath $qemuExe -ArgumentList $qemuArgs -PassThru
Start-Sleep -Seconds 30
Stop-Process -Id $proc.Id -Force -ErrorAction SilentlyContinue

if (Test-Path $serialLog) {
    $logSize = (Get-Item $serialLog).Length
    Write-Host "QEMU Serial Log Captured ($logSize bytes):" -ForegroundColor Green
    Write-Host "=== FILTERED [MEDIA-P2] TELEMETRY ===" -ForegroundColor Yellow
    Select-String -Path $serialLog -Pattern "\[MEDIA" | ForEach-Object { $_.Line }
} else {
    Write-Host "[ERROR] No serial log captured!" -ForegroundColor Red
}
