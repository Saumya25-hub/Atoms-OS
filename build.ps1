$ErrorActionPreference = "Stop"

# Ensure NASM and Clang are in path locally for the script
$env:Path = [Environment]::GetEnvironmentVariable("Path", "Machine") + ";" + [Environment]::GetEnvironmentVariable("Path", "User")

Write-Host "=========================================" -ForegroundColor Cyan
Write-Host "   Building SignaturesOS (Phase 2)       " -ForegroundColor Cyan
Write-Host "=========================================" -ForegroundColor Cyan

if (!(Test-Path "build")) {
    New-Item -ItemType Directory -Force -Path "build" | Out-Null
}

Write-Host "[1/3] Assembling Stage 1 Bootloader (boot.asm) -> boot.bin..." -ForegroundColor Yellow
nasm -f bin boot\boot.asm -o build\boot.bin

Write-Host "[2/3] Assembling Stage 2 Loader (stage2.asm) -> stage2.bin..." -ForegroundColor Yellow
nasm -f bin boot\stage2.asm -o build\stage2.bin

Write-Host "[3/3] Creating Bootable Disk Image (OS.img)..." -ForegroundColor Yellow
# Concatenate Stage 1 (Sector 1) and Stage 2 (Sector 2) into the raw image
cmd /c "copy /b build\boot.bin + build\stage2.bin build\OS.img > NUL"

Write-Host "=========================================" -ForegroundColor Green
Write-Host " BUILD SUCCESSFUL! Image: build\OS.img   " -ForegroundColor Green
Write-Host "=========================================" -ForegroundColor Green
