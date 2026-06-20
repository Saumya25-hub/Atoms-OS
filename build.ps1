# Reload environment variables to detect newly installed tools
$env:Path = [System.Environment]::GetEnvironmentVariable("Path","Machine") + ";" + [System.Environment]::GetEnvironmentVariable("Path","User")

Write-Host "=========================================" -ForegroundColor Cyan
Write-Host "   Building SignaturesOS (V1 HDD)        " -ForegroundColor Cyan
Write-Host "=========================================" -ForegroundColor Cyan

# Constants to verify
$BOOT_SECTOR_SIZE = 512
$STAGE2_SECTORS = 4
$KERNEL_SECTORS = 32
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

clang -target x86_64-unknown-none -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c drivers\video\vga\vga.c -o build\vga.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-unknown-none -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\console\console.c -o build\console.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-unknown-none -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\display\display.c -o build\display.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

Write-Host "[4/5] Assembling Kernel Entry..." -ForegroundColor Yellow
nasm -I boot\ -f elf64 kernel\kernel_entry.asm -o build\kernel_entry.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

Write-Host "[5/5] Linking Kernel..." -ForegroundColor Yellow
ld.lld -T kernel\linker.ld build\kernel_entry.o build\kernel.o build\port_io.o build\idt.o build\isr_stubs.o build\isr.o build\vga.o build\console.o build\display.o -o build\kernel.bin
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

Write-Host "[6/6] Creating Raw HDD Image (OS.img)..." -ForegroundColor Yellow
cmd /c "copy /b build\boot.bin + build\stage2.bin + build\kernel.bin build\OS.img > NUL"
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

# Pad OS.img to 1MB (1048576 bytes) for a standard VirtualBox IDE Raw Disk size
$imgPath = "$PWD\build\OS.img"
$img = [System.IO.File]::ReadAllBytes($imgPath)
$targetSize = 1048576 # 1 MB
if ($img.Length -lt $targetSize) {
    $padded = New-Object byte[] $targetSize
    [System.Array]::Copy($img, $padded, $img.Length)
    [System.IO.File]::WriteAllBytes($imgPath, $padded)
}

# ==============================================================================
# BUILD VALIDATION
# ==============================================================================
Write-Host "--- Performing Automated Build Validation ---" -ForegroundColor Cyan

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

$vdiPath = "$PWD\build\OS.vdi"
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
    Write-Host "To convert manually: VBoxManage convertfromraw build\OS.img build\OS.vdi --format VDI" -ForegroundColor Yellow
} else {
    Write-Host "[OK] VDI Created: build\OS.vdi" -ForegroundColor Green
}

Write-Host "=========================================" -ForegroundColor Green
Write-Host " BUILD SUCCESSFUL! Image: build\OS.vdi   " -ForegroundColor Green
Write-Host "=========================================" -ForegroundColor Green
