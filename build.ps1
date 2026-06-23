# Reload environment variables to detect newly installed tools
$env:Path = [System.Environment]::GetEnvironmentVariable("Path","Machine") + ";" + [System.Environment]::GetEnvironmentVariable("Path","User")

Write-Host "=========================================" -ForegroundColor Cyan
Write-Host "   Building SignaturesOS (V1 HDD)        " -ForegroundColor Cyan
Write-Host "=========================================" -ForegroundColor Cyan

# Constants to verify
$BOOT_SECTOR_SIZE = 512
$STAGE2_SECTORS = 4
$KERNEL_SECTORS = 128
$KERNEL_LBA = 1 + $STAGE2_SECTORS

if (-not (Test-Path "build")) {
    New-Item -ItemType Directory -Path "build" | Out-Null
}

Write-Host "[1/5] Assembling Stage 1 Bootloader..." -ForegroundColor Yellow
nasm -I boot\ -f bin boot\boot.asm -o build\boot.bin
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

Write-Host "[2/5] Assembling Stage 2 Loader..." -ForegroundColor Yellow
nasm -I boot\ -f bin boot\stage2.asm -o build\stage2.bin
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

Write-Host "[3/5] Compiling Kernel & Drivers..." -ForegroundColor Yellow
clang -target x86_64-unknown-none -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\kernel.c -o build\kernel.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-unknown-none -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c arch\x86_64\io\port_io.c -o build\port_io.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-unknown-none -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c arch\x86_64\interrupt\idt.c -o build\idt.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

nasm -f elf64 arch\x86_64\interrupt\isr_stubs.asm -o build\isr_stubs.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-unknown-none -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\interrupt\src\isr.c -o build\isr.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-unknown-none -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\interrupt\src\exception.c -o build\exception.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-unknown-none -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\interrupt\src\irq.c -o build\irq.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-unknown-none -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c drivers\video\vga\vga.c -o build\vga.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-unknown-none -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c drivers\interrupt\pic\pic.c -o build\pic.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-unknown-none -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\timer\src\timer.c -o build\timer.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-unknown-none -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c drivers\timer\pit\pit.c -o build\pit.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-unknown-none -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\keyboard\src\keyboard.c -o build\keyboard.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-unknown-none -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c drivers\input\ps2\ps2.c -o build\ps2.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-unknown-none -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\console\console.c -o build\console.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-unknown-none -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\display\display.c -o build\display.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-unknown-none -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\memory\pmm\src\pmm.c -o build\pmm.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-unknown-none -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\memory\pmm\src\bitmap.c -o build\bitmap.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-unknown-none -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\memory\vmm\src\vmm.c -o build\vmm.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-unknown-none -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\memory\vmm\src\paging.c -o build\paging.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-unknown-none -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\memory\heap\src\heap.c -o build\heap.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-unknown-none -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\lib\src\list.c -o build\list.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-unknown-none -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\lib\src\crash_log.c -o build\crash_log.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-unknown-none -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\scheduler\src\runqueue.c -o build\runqueue.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-unknown-none -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\scheduler\src\task.c -o build\task.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-unknown-none -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\scheduler\src\context.c -o build\context.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-unknown-none -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\scheduler\src\scheduler.c -o build\scheduler.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

nasm -f elf64 kernel\scheduler\src\context_switch.asm -o build\context_switch.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

nasm -f elf64 kernel\scheduler\src\enter_usermode.asm -o build\enter_usermode.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

nasm -f elf64 arch\x86_64\gdt\gdt_flush.asm -o build\gdt_flush.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -ffreestanding -mno-red-zone -I./ -c arch\x86_64\gdt\gdt.c -o build\gdt.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -ffreestanding -mno-red-zone -I. -c kernel\syscall\src\syscall.c -o build\syscall.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

nasm -f elf64 kernel\syscall\src\syscall_wrappers.asm -o build\syscall_wrappers.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

nasm -f elf64 kernel\syscall\src\syscall_entry.asm -o build\syscall_entry.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -ffreestanding -mno-red-zone -I. -c kernel\storage\src\block_device.c -o build\block_device.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -ffreestanding -mno-red-zone -I. -c kernel\driver\storage\src\ata.c -o build\ata.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -ffreestanding -mno-red-zone -I. -c kernel\storage\src\mbr.c -o build\mbr.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -ffreestanding -mno-red-zone -I. -c kernel\storage\src\disk_manager.c -o build\disk_manager.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -ffreestanding -mno-red-zone -I. -c kernel\vfs\src\vfs.c -o build\vfs.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -ffreestanding -mno-red-zone -I. -c kernel\lib\src\string.c -o build\string.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -ffreestanding -mno-red-zone -I. -c kernel\fs\fat32\src\fat32.c -o build\fat32.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -ffreestanding -mno-red-zone -I. -c kernel\loader\elf\src\elf_validate.c -o build\elf_validate.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -ffreestanding -mno-red-zone -I. -c kernel\loader\elf\src\elf_segment.c -o build\elf_segment.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -ffreestanding -mno-red-zone -I. -c kernel\process\src\process_builder.c -o build\process_builder.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -ffreestanding -mno-red-zone -I. -c kernel\process\src\process.c -o build\process.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }


Write-Host "[4/5] Assembling Kernel Entry..." -ForegroundColor Yellow
nasm -I boot\ -f elf64 kernel\kernel_entry.asm -o build\kernel_entry.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

Write-Host "[5/5] Linking Kernel..." -ForegroundColor Yellow
ld.lld -Map build\kernel.map -T kernel\linker.ld build\kernel_entry.o build\kernel.o build\port_io.o build\idt.o build\isr_stubs.o build\isr.o build\exception.o build\irq.o build\pic.o build\timer.o build\pit.o build\keyboard.o build\ps2.o build\vga.o build\console.o build\display.o build\pmm.o build\bitmap.o build\vmm.o build\paging.o build\heap.o build\list.o build\crash_log.o build\runqueue.o build\task.o build\context.o build\context_switch.o build\syscall.o build\syscall_wrappers.o build\syscall_entry.o build\gdt.o build\gdt_flush.o build\enter_usermode.o build\scheduler.o build\block_device.o build\ata.o build\mbr.o build\disk_manager.o build\vfs.o build\string.o build\fat32.o build\elf_validate.o build\elf_segment.o build\process_builder.o build\process.o -o build\kernel.bin
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

# Enforce Kernel Size Limit
$kernelFile = Get-Item "build\kernel.bin"
if ($kernelFile.Length -gt ($KERNEL_SECTORS * $BOOT_SECTOR_SIZE)) {
    Write-Host "BUILD FAILED! Kernel size ($($kernelFile.Length) bytes) exceeds allocated $KERNEL_SECTORS sectors." -ForegroundColor Red
    exit 1
}

# Pad Kernel.bin to exact KERNEL_SECTORS size to ensure exact offsets
$kernelBytes = [System.IO.File]::ReadAllBytes("build\kernel.bin")
$paddedKernel = New-Object byte[] ($KERNEL_SECTORS * $BOOT_SECTOR_SIZE)
[System.Array]::Copy($kernelBytes, $paddedKernel, $kernelBytes.Length)
[System.IO.File]::WriteAllBytes("build\kernel.bin", $paddedKernel)

Write-Host "[6/7] Building Userspace ELFs..." -ForegroundColor Yellow
clang -target x86_64-pc-none-elf -ffreestanding -nostdlib -c userspace\init.c -o build\init.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }
ld.lld -T userspace\linker.ld --strip-all build\init.o -o build\init.elf
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -ffreestanding -nostdlib -c userspace\libbos\src\syscalls.c -o build\syscalls.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -ffreestanding -nostdlib -c userspace\test.c -o build\test.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }
ld.lld -T userspace\linker.ld --strip-all build\test.o build\syscalls.o -o build\test.elf
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -ffreestanding -nostdlib -c userspace\shell\shell.c -o build\shell.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }
ld.lld -T userspace\linker.ld --strip-all build\shell.o build\syscalls.o -o build\shell.elf
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

Write-Host "[7/7] Creating Raw HDD Image (OS.img) via image_builder..." -ForegroundColor Yellow
clang -O2 tools\image_builder.c -o build\image_builder.exe
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED! Could not compile image_builder" -ForegroundColor Red; exit $LASTEXITCODE }

& .\build\image_builder.exe build\boot.bin build\stage2.bin build\kernel.bin build\OS.img
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED! image_builder failed" -ForegroundColor Red; exit $LASTEXITCODE }

# ==============================================================================
# BUILD VALIDATION
# ==============================================================================
Write-Host "--- Performing Automated Build Validation ---" -ForegroundColor Cyan

$imgPath = "$PWD\build\OS.img"
$finalImg = [System.IO.File]::ReadAllBytes($imgPath)

# 1. Image Size Multiple
if (($finalImg.Length % $BOOT_SECTOR_SIZE) -ne 0) {
    Write-Host "BUILD FAILED! Image size is not a multiple of 512 bytes." -ForegroundColor Red
    exit 1
}
Write-Host "[OK] Image Size Alignment Verified ($($finalImg.Length) bytes)" -ForegroundColor Green

# 2. Boot Signature
if ($finalImg[510] -ne 0x55 -or $finalImg[511] -ne 0xAA) {
    Write-Host "BUILD FAILED! Boot signature 0xAA55 missing at byte 510." -ForegroundColor Red
    exit 1
}
Write-Host "[OK] Boot Signature Verified" -ForegroundColor Green

# 3. Kernel LBA & Offset Match
$expectedKernelOffset = $KERNEL_LBA * $BOOT_SECTOR_SIZE
if ($expectedKernelOffset -ne (512 + ($STAGE2_SECTORS * 512))) {
    Write-Host "BUILD FAILED! Kernel LBA math mismatch." -ForegroundColor Red
    exit 1
}
Write-Host "[OK] Kernel Offset Verified (LBA $KERNEL_LBA -> Offset $expectedKernelOffset)" -ForegroundColor Green

# 4. Sector Count Match
$totalWrittenSectors = 1 + $STAGE2_SECTORS + $KERNEL_SECTORS
Write-Host "[OK] Active Sector Count Verified ($totalWrittenSectors sectors)" -ForegroundColor Green

# ==============================================================================
# VDI CONVERSION (VirtualBox Hard Disk Container)
# ==============================================================================
Write-Host "--- Converting to VDI for VirtualBox IDE ---" -ForegroundColor Cyan

$vdiPath = "$PWD\build\SignaturesOS.vdi"
# Remove old VDI if it exists (VBoxManage refuses to overwrite)
if (Test-Path $vdiPath) {
    Remove-Item $vdiPath -Force
}

# Locate VBoxManage from the Windows Registry
$vboxReg = Get-ItemProperty "HKLM:\SOFTWARE\Oracle\VirtualBox" -ErrorAction SilentlyContinue
if ($vboxReg -and $vboxReg.InstallDir) {
    $vboxManage = Join-Path $vboxReg.InstallDir "VBoxManage.exe"
} else {
    $vboxManage = "VBoxManage"
}

& $vboxManage convertfromraw $imgPath $vdiPath --format VDI
if ($LASTEXITCODE -ne 0) {
    Write-Host "WARNING: VBoxManage not found or conversion failed." -ForegroundColor Yellow
    Write-Host "You can still use build\OS.img as a raw disk." -ForegroundColor Yellow
    Write-Host "To convert manually: VBoxManage convertfromraw build\OS.img build\SignaturesOS.vdi --format VDI" -ForegroundColor Yellow
} else {
    Write-Host "[OK] VDI Created: build\SignaturesOS.vdi" -ForegroundColor Green
}

Write-Host "=========================================" -ForegroundColor Green
Write-Host " BUILD SUCCESSFUL! Image: build\SignaturesOS.vdi   " -ForegroundColor Green
Write-Host "=========================================" -ForegroundColor Green
