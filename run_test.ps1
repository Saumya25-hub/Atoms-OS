$secondaryDrive = ""
if (Test-Path "build\winxp_ntfs_real.raw") {
    $secondaryDrive = '-drive file="build\winxp_ntfs_real.raw",format=raw,index=1,media=disk'
} elseif (Test-Path "build\ntfs_real_test.raw") {
    $secondaryDrive = '-drive file="build\ntfs_real_test.raw",format=raw,index=1,media=disk'
}
Invoke-Expression "qemu-system-x86_64 -m 1024 -vga std -usb -device usb-tablet -drive file=""build\OS.img"",format=raw,index=0,media=disk $secondaryDrive -audiodev dsound,id=audio0 -device AC97,audiodev=audio0 -serial file:qemu_doom_test.log -display none -d guest_errors"
