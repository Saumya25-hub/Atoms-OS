$qemuExe = "D:\OS-QEMU-EMU\qemu\qemu-system-x86_64.exe"
$uefiBios = "D:\OS-QEMU-EMU\qemu\share\edk2-x86_64-code.fd"
$serialLog = "build\uefi_forensic_serial.log"

if (Test-Path $serialLog) { Remove-Item $serialLog }

$p = Start-Process -FilePath $qemuExe -ArgumentList "-drive if=pflash,format=raw,readonly=on,file=`"$uefiBios`" -drive file=build\atoms_uefi_test.img,format=raw -device qemu-xhci -device usb-mouse -device usb-kbd -serial file:$serialLog -m 2048M -display none -no-reboot" -PassThru
Start-Sleep -Seconds 25
if (-not $p.HasExited) {
    Stop-Process -Id $p.Id -Force
}
if (Test-Path $serialLog) {
    Get-Content $serialLog | Select-Object -Last 40
}
