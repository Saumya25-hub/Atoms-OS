$serialLog = 'build\phase1_qemu_serial.log'
if (Test-Path $serialLog) { Remove-Item $serialLog -Force }

$qemuExe = 'D:\OS-QEMU-EMU\qemu\qemu-system-x86_64.exe'
$biosFd = 'D:\OS-QEMU-EMU\qemu\share\edk2-x86_64-code.fd'

$qemuArgs = "-drive if=pflash,format=raw,readonly=on,file=$biosFd -drive file=build\atoms_uefi_test.img,format=raw -device qemu-xhci -device usb-kbd -device usb-mouse -audiodev none,id=audio0 -device intel-hda -device hda-duplex,audiodev=audio0 -serial file:$serialLog -m 2048M -display none -no-reboot"

Write-Host "Starting QEMU UEFI Pre-Flight Test..." -ForegroundColor Cyan
$proc = Start-Process -FilePath $qemuExe -ArgumentList $qemuArgs -PassThru
Start-Sleep -Seconds 25
Stop-Process -Id $proc.Id -Force -ErrorAction SilentlyContinue

if (Test-Path $serialLog) {
    Write-Host "QEMU Serial Log Captured ($((Get-Item $serialLog).Length) bytes):" -ForegroundColor Green
    Get-Content $serialLog -Tail 100
} else {
    Write-Host "[ERROR] No serial log captured!" -ForegroundColor Red
}
