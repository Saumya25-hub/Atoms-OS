$serialLog = 'build\uefi_forensic_serial.log'
if (Test-Path $serialLog) { Remove-Item $serialLog -Force }

$proc = Start-Process -FilePath 'D:\OS-QEMU-EMU\qemu\qemu-system-x86_64.exe' -ArgumentList '-drive if=pflash,format=raw,readonly=on,file=D:\OS-QEMU-EMU\qemu\share\edk2-x86_64-code.fd -drive file=build\atoms_uefi_test.img,format=raw -device qemu-xhci -device usb-kbd -device usb-mouse -serial file:build\uefi_forensic_serial.log -m 2048M -display none -no-reboot' -PassThru
Start-Sleep -Seconds 35
Stop-Process -Id $proc.Id -Force -ErrorAction SilentlyContinue

if (Test-Path $serialLog) {
    Get-Content $serialLog -Tail 60
} else {
    Write-Host "No serial log produced."
}
