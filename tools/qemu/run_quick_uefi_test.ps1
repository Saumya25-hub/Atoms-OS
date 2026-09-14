$qemuExe = "D:\OS-QEMU-EMU\qemu\qemu-system-x86_64.exe"
$uefiBios = "D:\OS-QEMU-EMU\qemu\share\edk2-x86_64-code.fd"
$serialLog = "build\uefi_forensic_serial.log"

if (Test-Path $serialLog) { Remove-Item $serialLog }

$process = Start-Process -FilePath $qemuExe -ArgumentList "-drive if=pflash,format=raw,readonly=on,file=`"$uefiBios`" -drive file=build\atoms_uefi_test.img,format=raw -device qemu-xhci -device usb-mouse -device usb-kbd -serial file:$serialLog -m 2048M -display none -no-reboot" -PassThru

Write-Host "QEMU started (PID: $($process.Id)). Waiting 38 seconds..."
Start-Sleep -Seconds 38

if (-not $process.HasExited) {
    Stop-Process -Id $process.Id -Force
}

Write-Host "================ SERIAL LOG OUTPUT ================"
if (Test-Path $serialLog) {
    Get-Content $serialLog
} else {
    Write-Host "Serial log not found!"
}
