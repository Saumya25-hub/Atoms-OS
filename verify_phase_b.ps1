$proc = Start-Process -FilePath "qemu-system-x86_64" -ArgumentList "-m 1024 -vga std -usb -device usb-tablet -drive file=build/OS.img,format=raw,index=0,media=disk -serial file:phase_b_test.log -display none" -PassThru
Start-Sleep -Seconds 10
if (!$proc.HasExited) { Stop-Process -Id $proc.Id -Force }
Get-Content phase_b_test.log -Tail 100
