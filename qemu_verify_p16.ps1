$qemu = 'C:\Program Files\qemu\qemu-system-x86_64.exe'
if (Test-Path 'qemu_phase16_boot.log') { Remove-Item 'qemu_phase16_boot.log' }
$proc = Start-Process -FilePath $qemu -ArgumentList '-cpu qemu64 -smp 1 -m 2048 -vga std -device qemu-xhci,id=xhci0 -device usb-mouse,bus=xhci0.0 -netdev user,id=net0 -device e1000,netdev=net0 -audiodev dsound,id=audio0 -device AC97,audiodev=audio0 -drive file=build\OS.img,format=raw,index=0,media=disk -serial file:qemu_phase16_boot.log -display none -no-reboot' -PassThru
Start-Sleep -Seconds 8
Stop-Process -Id $proc.Id -Force -ErrorAction SilentlyContinue
Start-Sleep -Seconds 1
if (Test-Path 'qemu_phase16_boot.log') {
    Get-Content 'qemu_phase16_boot.log' -Head 50
} else {
    Write-Host "Log not created"
}
