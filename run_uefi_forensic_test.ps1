# UEFI / GPT Forensic Boot Validation Script

Write-Host "=========================================" -ForegroundColor Cyan
Write-Host " UEFI/GPT FORENSIC BOOT VALIDATION RUN  " -ForegroundColor Cyan
Write-Host "=========================================" -ForegroundColor Cyan

# 0. Build Complete Kernel & Subsystems via build.ps1
Write-Host "[0/4] Building Complete SignaturesOS Kernel via build.ps1..." -ForegroundColor Yellow
& .\build.ps1
if ($LASTEXITCODE -ne 0) { Write-Host "Build failed!" -ForegroundColor Red; exit $LASTEXITCODE }

# 1. Compile BOOTX64.EFI
Write-Host "[1/4] Compiling BOOTX64.EFI..." -ForegroundColor Yellow
nasm -f win64 boot\uefi\kernel_payload.asm -o build\kernel_payload.o
clang -target x86_64-unknown-windows "-Wl,-subsystem:efi_application" "-Wl,-entry:efi_main" "-Wl,-dynamicbase:no" -mno-red-zone -mno-stack-arg-probe -nostdlib -ffreestanding -fno-stack-protector -fno-pic -I. boot\uefi\bootx64.c build\kernel_payload.o -o build\BOOTX64.EFI
if ($LASTEXITCODE -ne 0) { Write-Host "Compilation of BOOTX64.EFI failed!" -ForegroundColor Red; exit $LASTEXITCODE }

# 2. Compile gpt_image_builder.c
Write-Host "[2/4] Compiling GPT Image Builder..." -ForegroundColor Yellow
clang -O2 tools\gpt_image_builder.c -o build\gpt_image_builder.exe
if ($LASTEXITCODE -ne 0) { Write-Host "Compilation of gpt_image_builder failed!" -ForegroundColor Red; exit $LASTEXITCODE }

# 3. Generate atoms_uefi_test.img
Write-Host "[3/4] Generating GPT test image: build\atoms_uefi_test.img..." -ForegroundColor Yellow
& .\build\gpt_image_builder.exe build\BOOTX64.EFI build\kernel.bin build\atoms_uefi_test.img
if ($LASTEXITCODE -ne 0) { Write-Host "GPT test image generation failed!" -ForegroundColor Red; exit $LASTEXITCODE }

# 4. Launch QEMU with UEFI Firmware (OVMF / edk2-x86_64-code.fd)
Write-Host "[4/4] Launching QEMU in Pure UEFI Mode..." -ForegroundColor Yellow
$qemuExe = "D:\OS-QEMU-EMU\qemu\qemu-system-x86_64.exe"
$uefiBios = "D:\OS-QEMU-EMU\qemu\share\edk2-x86_64-code.fd"
$serialLog = "build\uefi_forensic_serial.log"

if (Test-Path $serialLog) { Remove-Item $serialLog }

$process = Start-Process -FilePath $qemuExe -ArgumentList "-drive if=pflash,format=raw,readonly=on,file=`"$uefiBios`" -drive file=build\atoms_uefi_test.img,format=raw -device qemu-xhci -device usb-mouse -device usb-kbd -serial file:$serialLog -m 512M -display none -no-reboot" -PassThru

# Wait 25 seconds for boot process to register all subsystem markers
Start-Sleep -Seconds 25

if (-not $process.HasExited) {
    Stop-Process -Id $process.Id -Force
}

Write-Host "=========================================" -ForegroundColor Green
Write-Host "        FORENSIC LOG ANALYSIS           " -ForegroundColor Green
Write-Host "=========================================" -ForegroundColor Green

if (Test-Path $serialLog) {
    Get-Content $serialLog
} else {
    Write-Host "Serial log not found!" -ForegroundColor Red
}
