$qemuExe = "D:\OS-QEMU-EMU\qemu\qemu-system-x86_64.exe"
$uefiBios = "D:\OS-QEMU-EMU\qemu\share\edk2-x86_64-code.fd"
$serialLog = "build\qemu_nvme_test.log"

if (Test-Path $serialLog) { Remove-Item $serialLog }

$qemuArgs = @(
    "-drive", "if=pflash,format=raw,readonly=on,file=$uefiBios",
    "-drive", "file=build\atoms_uefi_test.img,format=raw",
    "-drive", "file=build\OS.img,if=none,id=nvm0",
    "-device", "nvme,serial=ATOMS-TEST-NVME,drive=nvm0",
    "-device", "qemu-xhci", "-device", "usb-mouse", "-device", "usb-kbd",
    "-serial", "file:$serialLog",
    "-m", "2048M",
    "-display", "none",
    "-no-reboot"
)

$proc = Start-Process -FilePath $qemuExe -ArgumentList $qemuArgs -PassThru
Start-Sleep -Seconds 15

if (-not $proc.HasExited) {
    Stop-Process -Id $proc.Id -Force
}

if (Test-Path $serialLog) {
    Get-Content $serialLog -Tail 60
} else {
    Write-Host "Log file not found"
}
