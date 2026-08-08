# UEFI / GPT Forensic Boot Validation Script

Write-Host "=========================================" -ForegroundColor Cyan
Write-Host " UEFI/GPT FORENSIC BOOT VALIDATION RUN  " -ForegroundColor Cyan
Write-Host "=========================================" -ForegroundColor Cyan

# 0. Assemble kernel_entry.asm & Compile kernel.c & Link kernel.bin
Write-Host "[0/4] Assembling kernel_entry.asm..." -ForegroundColor Yellow
nasm -I boot\ -f elf64 kernel\kernel_entry.asm -o build\kernel_entry.o
if ($LASTEXITCODE -ne 0) { Write-Host "Assembly of kernel_entry.asm failed!" -ForegroundColor Red; exit $LASTEXITCODE }

Write-Host "[0.1/4] Assembling gdt_flush.asm, ap_trampoline.asm & isr_stubs.asm..." -ForegroundColor Yellow
nasm -f elf64 arch\x86_64\gdt\gdt_flush.asm -o build\gdt_flush.o
nasm -f elf64 arch\x86_64\smp\ap_trampoline.asm -o build\ap_trampoline.o
nasm -f elf64 arch\x86_64\interrupt\isr_stubs.asm -o build\isr_stubs.o

Write-Host "[0.2/4] Compiling ABDE V2.5 Engine, Drivers (PIC, IRQ, PS2, Keyboard, PIT) & kernel.c..." -ForegroundColor Yellow
clang -target x86_64-unknown-none-elf -mno-red-zone -mno-stack-arg-probe -nostdlib -ffreestanding -fno-stack-protector -fno-pic -I. -c kernel\debug\abde\abde_font.c -o build\abde_font.o
clang -target x86_64-unknown-none-elf -mno-red-zone -mno-stack-arg-probe -nostdlib -ffreestanding -fno-stack-protector -fno-pic -I. -c kernel\debug\abde\abde_renderer.c -o build\abde_renderer.o
clang -target x86_64-unknown-none-elf -mno-red-zone -mno-stack-arg-probe -nostdlib -ffreestanding -fno-stack-protector -fno-pic -I. -c kernel\debug\abde\abde.c -o build\abde.o
clang -target x86_64-unknown-none-elf -mno-red-zone -mno-stack-arg-probe -nostdlib -ffreestanding -fno-stack-protector -fno-pic -I. -c arch\x86_64\cpu\cpu_features.c -o build\cpu_features.o
clang -target x86_64-unknown-none-elf -mno-red-zone -mno-stack-arg-probe -nostdlib -ffreestanding -fno-stack-protector -fno-pic -I. -c arch\x86_64\gdt\gdt.c -o build\gdt.o
clang -target x86_64-unknown-none-elf -mno-red-zone -mno-stack-arg-probe -nostdlib -ffreestanding -fno-stack-protector -fno-pic -I. -c arch\x86_64\smp\smp.c -o build\smp.o
clang -target x86_64-unknown-none-elf -mno-red-zone -mno-stack-arg-probe -nostdlib -ffreestanding -fno-stack-protector -fno-pic -I. -c arch\x86_64\interrupt\idt.c -o build\idt.o
clang -target x86_64-unknown-none-elf -mno-red-zone -mno-stack-arg-probe -nostdlib -ffreestanding -fno-stack-protector -fno-pic -I. -c arch\x86_64\io\port_io.c -o build\port_io.o
clang -target x86_64-unknown-none-elf -mno-red-zone -mno-stack-arg-probe -nostdlib -ffreestanding -fno-stack-protector -fno-pic -I. -c drivers\interrupt\pic\pic.c -o build\pic.o
clang -target x86_64-unknown-none-elf -mno-red-zone -mno-stack-arg-probe -nostdlib -ffreestanding -fno-stack-protector -fno-pic -I. -c kernel\core\interrupt\src\isr.c -o build\isr.o
clang -target x86_64-unknown-none-elf -mno-red-zone -mno-stack-arg-probe -nostdlib -ffreestanding -fno-stack-protector -fno-pic -I. -c kernel\core\interrupt\src\irq.c -o build\irq.o
clang -target x86_64-unknown-none-elf -mno-red-zone -mno-stack-arg-probe -nostdlib -ffreestanding -fno-stack-protector -fno-pic -I. -c kernel\core\interrupt\src\exception.c -o build\exception.o
clang -target x86_64-unknown-none-elf -mno-red-zone -mno-stack-arg-probe -nostdlib -ffreestanding -fno-stack-protector -fno-pic -I. -c drivers\input\ps2\ps2.c -o build\ps2.o
clang -target x86_64-unknown-none-elf -mno-red-zone -mno-stack-arg-probe -nostdlib -ffreestanding -fno-stack-protector -fno-pic -I. -c kernel\drivers\keyboard\src\keyboard.c -o build\keyboard.o
clang -target x86_64-unknown-none-elf -mno-red-zone -mno-stack-arg-probe -nostdlib -ffreestanding -fno-stack-protector -fno-pic -I. -c drivers\timer\pit\pit.c -o build\pit.o
clang -target x86_64-unknown-none-elf -mno-red-zone -mno-stack-arg-probe -nostdlib -ffreestanding -fno-stack-protector -fno-pic -I. -c kernel\core\memory\pmm\src\bitmap.c -o build\bitmap.o
clang -target x86_64-unknown-none-elf -mno-red-zone -mno-stack-arg-probe -nostdlib -ffreestanding -fno-stack-protector -fno-pic -I. -c kernel\core\memory\pmm\src\pmm.c -o build\pmm.o
clang -target x86_64-unknown-none-elf -mno-red-zone -mno-stack-arg-probe -nostdlib -ffreestanding -fno-stack-protector -fno-pic -I. -c kernel\kernel.c -o build\kernel.o
if ($LASTEXITCODE -ne 0) { Write-Host "Compilation failed!" -ForegroundColor Red; exit $LASTEXITCODE }

Write-Host "[0.5/4] Linking kernel.bin..." -ForegroundColor Yellow
ld.lld @build\link.rsp -o build\kernel.bin
if ($LASTEXITCODE -ne 0) { Write-Host "Linking of kernel.bin failed!" -ForegroundColor Red; exit $LASTEXITCODE }

# 1. Compile BOOTX64.EFI
Write-Host "[1/4] Compiling BOOTX64.EFI..." -ForegroundColor Yellow
clang -target x86_64-unknown-windows "-Wl,-subsystem:efi_application" "-Wl,-entry:efi_main" "-Wl,-dynamicbase:no" -mno-red-zone -mno-stack-arg-probe -nostdlib -ffreestanding -fno-stack-protector -fno-pic -I. boot\uefi\bootx64.c -o build\BOOTX64.EFI
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

$process = Start-Process -FilePath $qemuExe -ArgumentList "-drive if=pflash,format=raw,readonly=on,file=`"$uefiBios`" -drive file=build\atoms_uefi_test.img,format=raw -serial file:$serialLog -m 512M -display none -no-reboot" -PassThru

# Wait 8 seconds for boot process to register markers
Start-Sleep -Seconds 8

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
