$qemu = "qemu-system-x86_64"
$proc = Start-Process -FilePath $qemu -ArgumentList "-m 1024 -vga std -usb -device usb-tablet -drive file=build\OS.img,format=raw,index=0,media=disk -audiodev none,id=audio0 -device AC97,audiodev=audio0 -serial file:qemu_doom_test11.log -display none" -PassThru
Start-Sleep -Seconds 60
if (-not $proc.HasExited) {
    Stop-Process -Id $proc.Id -Force
}
