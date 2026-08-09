# ATOMS OS — H81 Physical Hardware Certification Image Generator Script
# Produces: build/ATOMS_HEAP_H81_CERTIFICATION.img

Write-Host "=========================================================" -ForegroundColor Cyan
Write-Host " ATOMS OS -- HEAP H81 HARDWARE CERTIFICATION IMAGE BUILD " -ForegroundColor Cyan
Write-Host "=========================================================" -ForegroundColor Cyan

# 0. Assemble Kernel Entry Stubs
Write-Host "[1/5] Assembling low-level assembly stubs..." -ForegroundColor Yellow
nasm -I boot\ -f elf64 kernel\kernel_entry.asm -o build\kernel_entry.o
nasm -f elf64 arch\x86_64\gdt\gdt_flush.asm -o build\gdt_flush.o
nasm -f elf64 arch\x86_64\smp\ap_trampoline.asm -o build\ap_trampoline.o
nasm -f elf64 arch\x86_64\interrupt\isr_stubs.asm -o build\isr_stubs.o
nasm -f elf64 kernel\core\scheduler\src\context_switch.asm -o build\context_switch.o
if ($LASTEXITCODE -ne 0) { Write-Host "Assembly failed!" -ForegroundColor Red; exit $LASTEXITCODE }

# 1. Compile C Modules
Write-Host "[2/5] Compiling ABDE V2.5 Engine, Memory Stack, Scheduler and kernel.c..." -ForegroundColor Yellow
clang -target x86_64-unknown-none-elf -mno-red-zone -mno-stack-arg-probe -nostdlib -ffreestanding -fno-stack-protector -fno-pic -I. -c kernel\debug\abde\abde_font.c -o build\abde_font.o
clang -target x86_64-unknown-none-elf -mno-red-zone -mno-stack-arg-probe -nostdlib -ffreestanding -fno-stack-protector -fno-pic -I. -c kernel\debug\abde\abde_renderer.c -o build\abde_renderer.o
clang -target x86_64-unknown-none-elf -mno-red-zone -mno-stack-arg-probe -nostdlib -ffreestanding -fno-stack-protector -fno-pic -I. -c kernel\debug\abde\abde.c -o build\abde.o
clang -target x86_64-unknown-none-elf -mno-red-zone -mno-stack-arg-probe -nostdlib -ffreestanding -fno-stack-protector -fno-pic -I. -c arch\x86_64\cpu\cpu_features.c -o build\cpu_features.o
clang -target x86_64-unknown-none-elf -mno-red-zone -mno-stack-arg-probe -nostdlib -ffreestanding -fno-stack-protector -fno-pic -I. -c kernel\core\cpu\cpu_state.c -o build\cpu_state.o
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
clang -target x86_64-unknown-none-elf -mno-red-zone -mno-stack-arg-probe -nostdlib -ffreestanding -fno-stack-protector -fno-pic -I. -c kernel\core\timer\src\timer.c -o build\timer.o
clang -target x86_64-unknown-none-elf -mno-red-zone -mno-stack-arg-probe -nostdlib -ffreestanding -fno-stack-protector -fno-pic -I. -c kernel\core\memory\pmm\src\bitmap.c -o build\bitmap.o
clang -target x86_64-unknown-none-elf -mno-red-zone -mno-stack-arg-probe -nostdlib -ffreestanding -fno-stack-protector -fno-pic -I. -c kernel\core\memory\pmm\src\pmm.c -o build\pmm.o
clang -target x86_64-unknown-none-elf -mno-red-zone -mno-stack-arg-probe -nostdlib -ffreestanding -fno-stack-protector -fno-pic -I. -c kernel\core\memory\vmm\src\paging.c -o build\paging.o
clang -target x86_64-unknown-none-elf -mno-red-zone -mno-stack-arg-probe -nostdlib -ffreestanding -fno-stack-protector -fno-pic -I. -c kernel\core\memory\vmm\src\vmm.c -o build\vmm.o
clang -target x86_64-unknown-none-elf -mno-red-zone -mno-stack-arg-probe -nostdlib -ffreestanding -fno-stack-protector -fno-pic -I. -c kernel\core\memory\heap\src\heap.c -o build\heap.o
clang -target x86_64-unknown-none-elf -mno-red-zone -mno-stack-arg-probe -nostdlib -ffreestanding -fno-stack-protector -fno-pic -I. -c kernel\core\scheduler\src\runqueue.c -o build\runqueue.o
clang -target x86_64-unknown-none-elf -mno-red-zone -mno-stack-arg-probe -nostdlib -ffreestanding -fno-stack-protector -fno-pic -I. -c kernel\core\scheduler\src\task.c -o build\task.o
clang -target x86_64-unknown-none-elf -mno-red-zone -mno-stack-arg-probe -nostdlib -ffreestanding -fno-stack-protector -fno-pic -I. -c kernel\core\scheduler\src\context.c -o build\context.o
clang -target x86_64-unknown-none-elf -mno-red-zone -mno-stack-arg-probe -nostdlib -ffreestanding -fno-stack-protector -fno-pic -I. -c kernel\core\scheduler\src\scheduler.c -o build\scheduler.o
clang -target x86_64-unknown-none-elf -mno-red-zone -mno-stack-arg-probe -nostdlib -ffreestanding -fno-stack-protector -fno-pic -I. -c kernel\core\thread\thread_manager.c -o build\thread_manager.o
clang -target x86_64-unknown-none-elf -mno-red-zone -mno-stack-arg-probe -nostdlib -ffreestanding -fno-stack-protector -fno-pic -I. -c kernel\shell\debug_shell.c -o build\debug_shell.o
clang -target x86_64-unknown-none-elf -mno-red-zone -mno-stack-arg-probe -nostdlib -ffreestanding -fno-stack-protector -fno-pic -I. -c kernel\shell\console\console.c -o build\console.o
clang -target x86_64-unknown-none-elf -mno-red-zone -mno-stack-arg-probe -nostdlib -ffreestanding -fno-stack-protector -fno-pic -I. -c kernel\drivers\display\display.c -o build\display.o
clang -target x86_64-unknown-none-elf -mno-red-zone -mno-stack-arg-probe -nostdlib -ffreestanding -fno-stack-protector -fno-pic -I. -c kernel\kernel.c -o build\kernel.o
if ($LASTEXITCODE -ne 0) { Write-Host "Compilation failed!" -ForegroundColor Red; exit $LASTEXITCODE }

# 2. Link kernel.bin
Write-Host "[3/5] Linking kernel.bin..." -ForegroundColor Yellow
ld.lld @build\link.rsp -o build\kernel.bin
if ($LASTEXITCODE -ne 0) { Write-Host "Linking failed!" -ForegroundColor Red; exit $LASTEXITCODE }

# 3. Compile BOOTX64.EFI
Write-Host "[4/5] Compiling UEFI Bootloader (BOOTX64.EFI)..." -ForegroundColor Yellow
clang -target x86_64-unknown-windows "-Wl,-subsystem:efi_application" "-Wl,-entry:efi_main" "-Wl,-dynamicbase:no" -mno-red-zone -mno-stack-arg-probe -nostdlib -ffreestanding -fno-stack-protector -fno-pic -I. boot\uefi\bootx64.c -o build\BOOTX64.EFI
if ($LASTEXITCODE -ne 0) { Write-Host "BOOTX64.EFI compilation failed!" -ForegroundColor Red; exit $LASTEXITCODE }

# 4. Generate ATOMS_HEAP_H81_CERTIFICATION.img
Write-Host "[5/5] Generating GPT image: build\ATOMS_HEAP_H81_CERTIFICATION.img..." -ForegroundColor Yellow
clang -O2 tools\gpt_image_builder.c -o build\gpt_image_builder.exe
& .\build\gpt_image_builder.exe build\BOOTX64.EFI build\kernel.bin build\ATOMS_HEAP_H81_CERTIFICATION.img
if ($LASTEXITCODE -ne 0) { Write-Host "Image generation failed!" -ForegroundColor Red; exit $LASTEXITCODE }

Write-Host "=========================================================" -ForegroundColor Green
Write-Host " SUCCESS: CREATED build/ATOMS_HEAP_H81_CERTIFICATION.img " -ForegroundColor Green
Write-Host " Ready for Rufus USB Flashing and Intel H81 Bare-Metal Test! " -ForegroundColor Green
Write-Host "=========================================================" -ForegroundColor Green
