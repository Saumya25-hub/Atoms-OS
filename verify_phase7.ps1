$proc = Start-Process -FilePath "qemu-system-x86_64" -ArgumentList "-m 2048 -vga std -usb -device usb-tablet -drive file=build/OS.img,format=raw,index=0,media=disk -serial file:phase7_qemu.log -display none" -PassThru
Start-Sleep -Seconds 12
if (!$proc.HasExited) {
    Stop-Process -Id $proc.Id -Force
}
if (Test-Path "phase7_qemu.log") {
    Get-Content phase7_qemu.log -Tail 100
} else {
    Write-Host "No phase7_qemu.log produced."
}
