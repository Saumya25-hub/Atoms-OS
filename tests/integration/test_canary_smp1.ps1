$qemu = "D:\OS-QEMU-EMU\qemu\qemu-system-x86_64.exe"
if (!(Test-Path $qemu)) {
    $qemu = "qemu-system-x86_64"
}
$proc = Start-Process -FilePath $qemu -ArgumentList "-drive file=build/OS.img,format=raw,index=0,media=disk -m 512M -smp 1 -vga std -audiodev none,id=audio0 -device AC97,audiodev=audio0 -serial file:test_smp1.log -display none" -PassThru
Start-Sleep -Seconds 90
if (!$proc.HasExited) {
    Stop-Process -Id $proc.Id -Force
}
