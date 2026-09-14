$proc = Start-Process -FilePath "qemu-system-x86_64" -ArgumentList "-drive file=build/OS.img,format=raw,index=0,media=disk -m 512M -vga std -audiodev none,id=audio0 -device AC97,audiodev=audio0 -serial file:forensic_trace.log -display none" -PassThru
Start-Sleep -Seconds 60
if (!$proc.HasExited) {
    Stop-Process -Id $proc.Id -Force
}
