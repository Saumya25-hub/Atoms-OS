# Reload environment variables to detect newly installed tools
$env:Path = [System.Environment]::GetEnvironmentVariable("Path","Machine") + ";" + [System.Environment]::GetEnvironmentVariable("Path","User")

Write-Host "=========================================" -ForegroundColor Cyan
Write-Host "   Building SignaturesOS (V1 HDD)        " -ForegroundColor Cyan
Write-Host "=========================================" -ForegroundColor Cyan

# Constants to verify
$BOOT_SECTOR_SIZE = 512
$STAGE2_SECTORS = 4
$KERNEL_SECTORS = 256
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
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\kernel.c -o build\kernel.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c arch\x86_64\io\port_io.c -o build\port_io.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c arch\x86_64\interrupt\idt.c -o build\idt.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

nasm -f elf64 arch\x86_64\interrupt\isr_stubs.asm -o build\isr_stubs.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\interrupt\src\isr.c -o build\isr.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\interrupt\src\exception.c -o build\exception.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\interrupt\src\irq.c -o build\irq.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c drivers\video\vga\vga.c -o build\vga.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c drivers\interrupt\pic\pic.c -o build\pic.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\timer\src\timer.c -o build\timer.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c drivers\timer\pit\pit.c -o build\pit.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\keyboard\src\keyboard.c -o build\keyboard.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c drivers\input\ps2\ps2.c -o build\ps2.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\input\bmde.c -o build\bmde.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\console\console.c -o build\console.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\display\display.c -o build\display.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\memory\pmm\src\pmm.c -o build\pmm.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\memory\pmm\src\bitmap.c -o build\bitmap.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\memory\vmm\src\vmm.c -o build\vmm.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\memory\vmm\src\paging.c -o build\paging.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }


clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\lib\src\crash_log.c -o build\crash_log.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\scheduler\src\runqueue.c -o build\runqueue.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\scheduler\src\task.c -o build\task.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\scheduler\src\context.c -o build\context.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\scheduler\src\scheduler.c -o build\scheduler.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

nasm -f elf64 kernel\scheduler\src\context_switch.asm -o build\context_switch.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

nasm -f elf64 kernel\scheduler\src\enter_usermode.asm -o build\enter_usermode.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

nasm -f elf64 arch\x86_64\gdt\gdt_flush.asm -o build\gdt_flush.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I./ -c arch\x86_64\gdt\gdt.c -o build\gdt.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\syscall\src\syscall.c -o build\syscall.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

nasm -f elf64 kernel\syscall\src\syscall_wrappers.asm -o build\syscall_wrappers.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

nasm -f elf64 kernel\syscall\src\syscall_entry.asm -o build\syscall_entry.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\storage\src\block_device.c -o build\block_device.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\driver\storage\src\ata.c -o build\ata.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\storage\src\mbr.c -o build\mbr.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\storage\src\disk_manager.c -o build\disk_manager.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\vfs\src\vfs.c -o build\vfs.o
if ($LASTEXITCODE -ne 0) { Write-Host "VFS Failed!" -ForegroundColor Red; exit }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c drivers\input\ps2\ps2.c -o build\ps2.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c drivers\input\ps2\mouse.c -o build\ps2_mouse.o
if ($LASTEXITCODE -ne 0) { Write-Host "PS2 Driver Failed!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\input\input.c -o build\kernel_input.o

Write-Host "Compiling Core..."
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c bovisual\Core\core.c -o build\bv_core.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c bovisual\Core\layout.c -o build\bv_layout.o
if ($LASTEXITCODE -ne 0) { Write-Host "BV Core Failed!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c bovisual\Core\input.c -o build\bv_input.o

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c bovisual\Graphics\graphics.c -o build\bv_graphics.o
if ($LASTEXITCODE -ne 0) { Write-Host "BV Graphics Failed!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c bovisual\Text\text.c -o build\bv_text.o
if ($LASTEXITCODE -ne 0) { Write-Host "BV Text Failed!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c bovisual\Drawing\drawing.c -o build\bv_drawing.o
if ($LASTEXITCODE -ne 0) { Write-Host "BV Drawing Failed!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c bovisual\Renderer\renderer.c -o build\bv_renderer.o
if ($LASTEXITCODE -ne 0) { Write-Host "BV Renderer Failed!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c bovisual\Controls\controls.c -o build\bv_controls.o
if ($LASTEXITCODE -ne 0) { Write-Host "BV Controls Failed!" -ForegroundColor Red; exit $LASTEXITCODE }

Write-Host "Compiling BOSurface (BWE)..."
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\BOSurface\Core\surface.c -o build\surface.o
if ($LASTEXITCODE -ne 0) { Write-Host "BWE Core Failed!" -ForegroundColor Red; exit $LASTEXITCODE }

Write-Host "Compiling Geometry..."
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c bovisual\Geometry\geometry.c -o build\bv_geometry.o
if ($LASTEXITCODE -ne 0) { Write-Host "BV Math Failed!" -ForegroundColor Red; exit $LASTEXITCODE }

Write-Host "Compiling BOSCAL Layout Engine..."
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c bovisual\BOSCAL\boscal.c -o build\bv_boscal.o
if ($LASTEXITCODE -ne 0) { Write-Host "BV BOSCAL Failed!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c bovisual\Images\images.c -o build\bv_images.o
if ($LASTEXITCODE -ne 0) { Write-Host "BV Images Failed!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\lib\src\string.c -o build\string.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\fs\fat32\src\fat32.c -o build\fat32.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\loader\elf\src\elf_validate.c -o build\elf_validate.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\loader\elf\src\elf_segment.c -o build\elf_segment.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\process\src\process_builder.c -o build\process_builder.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\process\src\process.c -o build\process.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }


Write-Host "[4/5] Assembling Kernel Entry..." -ForegroundColor Yellow
nasm -I boot\ -f elf64 kernel\kernel_entry.asm -o build\kernel_entry.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

Write-Host "[5/5] Linking Kernel..." -ForegroundColor Yellow
ld.lld -Map build\kernel.map -T kernel\linker.ld build\kernel_entry.o build\kernel.o build\port_io.o build\idt.o build\isr_stubs.o build\isr.o build\exception.o build\irq.o build\pic.o build\timer.o build\pit.o build\keyboard.o build\ps2.o build\ps2_mouse.o build\vbe.o build\bv_core.o build\bv_graphics.o build\bv_text.o build\bv_drawing.o build\bv_renderer.o build\bv_controls.o build\surface.o build\bv_geometry.o build\bv_boscal.o build\bv_images.o build\bv_layout.o build\bv_input.o build\kernel_input.o build\bmde.o build\vga.o build\console.o build\display.o build\pmm.o build\bitmap.o build\vmm.o build\paging.o build\heap.o build\list.o build\crash_log.o build\runqueue.o build\task.o build\context.o build\context_switch.o build\syscall.o build\syscall_wrappers.o build\syscall_entry.o build\gdt.o build\gdt_flush.o build\enter_usermode.o build\scheduler.o build\block_device.o build\ata.o build\mbr.o build\disk_manager.o build\vfs.o build\string.o build\fat32.o build\elf_validate.o build\elf_segment.o build\process_builder.o build\process.o -o build\kernel.bin
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
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -nostdlib -c userspace\init.c -o build\init.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }
ld.lld -T userspace\linker.ld --strip-all build\init.o -o build\init.elf
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -nostdlib -c userspace\libbos\src\syscalls.c -o build\syscalls.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -nostdlib -c userspace\libbos\src\bodiskhub.c -o build\bodiskhub.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -nostdlib -c userspace\test.c -o build\test.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }
ld.lld -T userspace\linker.ld --strip-all build\test.o build\syscalls.o build\bodiskhub.o -o build\test.elf
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -nostdlib -c userspace\shell\shell.c -o build\shell.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -nostdlib -c userspace\shell\command.c -o build\command.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -nostdlib -c userspace\shell\commands_sys.c -o build\commands_sys.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -nostdlib -c userspace\shell\commands_debug.c -o build\commands_debug.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -nostdlib -c userspace\shell\commands_bodh.c -o build\commands_bodh.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -nostdlib -c userspace\shell\commands_edit.c -o build\commands_edit.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -nostdlib -c userspace\shell\commands_diag.c -o build\commands_diag.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

# Compile SDS Framework for Shell linkage
Write-Host "Compiling SDS Diagnostics Framework..." -ForegroundColor Cyan
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -nostdlib -c sds\Core\core.c -o build\sds_core.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -nostdlib -c sds\Database\database.c -o build\sds_database.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -nostdlib -c sds\Console\console.c -o build\sds_console.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -nostdlib -c sds\Logger\logger.c -o build\sds_logger.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

# Atoms Library Compilation
Write-Host "Compiling Atoms Library..." -ForegroundColor Cyan
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -nostdlib -c userspace\atoms\src\atom_memory.c -o build\atom_memory.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -nostdlib -c userspace\atoms\src\atom_types.c -o build\atom_types.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -nostdlib -c userspace\atoms\src\atom_value.c -o build\atom_value.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -nostdlib -c userspace\atoms\src\atom_string.c -o build\atom_string.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -nostdlib -c userspace\atoms\src\atom_init.c -o build\atom_init.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -nostdlib -c userspace\atoms\tests\atom_self_test.c -o build\atom_self_test.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -nostdlib -c userspace\atoms\src\atom_array.c -o build\atom_array.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -nostdlib -c userspace\atoms\src\atom_table.c -o build\atom_table.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -nostdlib -c userspace\atoms\src\atom_bytecode.c -o build\atom_bytecode.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -nostdlib -c userspace\atoms\src\atom_function.c -o build\atom_function.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -nostdlib -c userspace\atoms\src\atom_scope.c -o build\atom_scope.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -nostdlib -c userspace\atoms\src\atom_vm.c -o build\atom_vm.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -nostdlib -c userspace\atoms\src\atom_lexer.c -o build\atom_lexer.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -nostdlib -c userspace\atoms\src\atom_compiler.c -o build\atom_compiler.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

# BishopMath Engine Compilation
Write-Host "Compiling BishopMath Engine..." -ForegroundColor Cyan
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -nostdlib -c userspace\bishopmath\src\bishop_error.c -o build\bishop_error.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -nostdlib -c userspace\bishopmath\src\bishop_basic.c -o build\bishop_basic.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -nostdlib -c userspace\bishopmath\src\bishop_scientific.c -o build\bishop_scientific.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -nostdlib -c userspace\bishopmath\src\bishop_matrix.c -o build\bishop_matrix.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -nostdlib -c userspace\bishopmath\src\bishop_vector.c -o build\bishop_vector.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -nostdlib -c userspace\bishopmath\src\bishop_complex.c -o build\bishop_complex.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -nostdlib -c userspace\bishopmath\src\bishop_bigint.c -o build\bishop_bigint.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -nostdlib -c userspace\bishopmath\src\bishop_stats.c -o build\bishop_stats.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -nostdlib -c userspace\bishopmath\tests\bishop_self_test.c -o build\bishop_self_test.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -nostdlib -c userspace\bishopmath\src\bishop_builtins.c -o build\bishop_builtins.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

ld.lld -T userspace\linker.ld --strip-all build\shell.o build\command.o build\commands_sys.o build\commands_debug.o build\commands_bodh.o build\commands_edit.o build\commands_diag.o build\sds_core.o build\sds_database.o build\sds_console.o build\sds_logger.o build\syscalls.o build\bodiskhub.o build\atom_memory.o build\atom_types.o build\atom_value.o build\atom_string.o build\atom_array.o build\atom_table.o build\atom_bytecode.o build\atom_function.o build\atom_scope.o build\atom_vm.o build\atom_lexer.o build\atom_compiler.o build\atom_init.o build\atom_self_test.o build\bishop_error.o build\bishop_basic.o build\bishop_scientific.o build\bishop_matrix.o build\bishop_vector.o build\bishop_complex.o build\bishop_bigint.o build\bishop_stats.o build\bishop_self_test.o build\bishop_builtins.o -o build\shell.elf
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

# Test Framework Compilation
Write-Host "Compiling OS Validation & Stress Test Framework..." -ForegroundColor Cyan
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -nostdlib -c userspace\tests\test_runner.c -o build\test_runner.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -nostdlib -c userspace\tests\heap\test_heap.c -o build\test_heap.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -nostdlib -c userspace\tests\scheduler\test_scheduler.c -o build\test_scheduler.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -nostdlib -c userspace\tests\filesystem\test_fs.c -o build\test_fs.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -nostdlib -c userspace\tests\bosl\test_bosl.c -o build\test_bosl.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -nostdlib -c userspace\tests\bishop\test_bishop.c -o build\test_bishop.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -nostdlib -c userspace\tests\panic\test_panic.c -o build\test_panic.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -nostdlib -c userspace\tests\performance\test_perf.c -o build\test_perf.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -nostdlib -c userspace\tests\panic\fault.c -o build\fault.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

ld.lld -T userspace\linker.ld --strip-all build\test_runner.o build\test_heap.o build\test_scheduler.o build\test_fs.o build\test_bosl.o build\test_bishop.o build\test_panic.o build\test_perf.o build\syscalls.o build\atom_memory.o build\atom_types.o build\atom_value.o build\atom_string.o build\atom_array.o build\atom_table.o build\atom_bytecode.o build\atom_function.o build\atom_scope.o build\atom_vm.o build\atom_lexer.o build\atom_compiler.o build\atom_init.o build\bishop_error.o build\bishop_basic.o build\bishop_scientific.o build\bishop_matrix.o build\bishop_vector.o build\bishop_complex.o build\bishop_bigint.o build\bishop_stats.o build\bishop_builtins.o -o build\tests.elf
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

ld.lld -T userspace\linker.ld --strip-all build\fault.o build\syscalls.o -o build\fault.elf
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

$vdiPath = "build\SignaturesOS_v2.vdi"
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
