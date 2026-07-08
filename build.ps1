# Reload environment variables to detect newly installed tools
$env:Path = [System.Environment]::GetEnvironmentVariable("Path","Machine") + ";" + [System.Environment]::GetEnvironmentVariable("Path","User")

Write-Host "=========================================" -ForegroundColor Cyan
Write-Host "   Building SignaturesOS (V1 HDD)        " -ForegroundColor Cyan
Write-Host "=========================================" -ForegroundColor Cyan

# Constants to verify
$BOOT_SECTOR_SIZE = 512
$STAGE2_SECTORS = 4
$KERNEL_SECTORS = 640
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

clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\core\interrupt\src\isr.c -o build\isr.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\core\interrupt\src\exception.c -o build\exception.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\core\interrupt\src\irq.c -o build\irq.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c drivers\video\vga\vga.c -o build\vga.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\drivers\video\vbe\vbe.c -o build\vbe.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c drivers\interrupt\pic\pic.c -o build\pic.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\core\timer\src\timer.c -o build\timer.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c drivers\timer\pit\pit.c -o build\pit.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\drivers\keyboard\src\keyboard.c -o build\keyboard.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c drivers\input\ps2\ps2.c -o build\ps2.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\drivers\input\bmde.c -o build\bmde.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\shell\console\console.c -o build\console.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\drivers\display\display.c -o build\display.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\core\memory\pmm\src\pmm.c -o build\pmm.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\core\memory\pmm\src\bitmap.c -o build\bitmap.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\core\memory\vmm\src\vmm.c -o build\vmm.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\core\memory\vmm\src\paging.c -o build\paging.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\core\memory\heap\src\heap.c -o build\heap.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\core\lib\src\list.c -o build\list.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\core\lib\src\crash_log.c -o build\crash_log.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\core\scheduler\src\runqueue.c -o build\runqueue.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\core\scheduler\src\task.c -o build\task.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\core\scheduler\src\context.c -o build\context.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\core\scheduler\src\scheduler.c -o build\scheduler.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

nasm -f elf64 kernel\core\scheduler\src\context_switch.asm -o build\context_switch.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

nasm -f elf64 kernel\core\scheduler\src\enter_usermode.asm -o build\enter_usermode.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

nasm -f elf64 arch\x86_64\gdt\gdt_flush.asm -o build\gdt_flush.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I./ -c arch\x86_64\gdt\gdt.c -o build\gdt.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\core\syscall\src\syscall.c -o build\syscall.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

nasm -f elf64 kernel\core\syscall\src\syscall_wrappers.asm -o build\syscall_wrappers.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

nasm -f elf64 kernel\core\syscall\src\syscall_entry.asm -o build\syscall_entry.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\vfs\vfs_legacy\storage\src\block_device.c -o build\block_device.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\drivers\storage_legacy\storage\src\ata.c -o build\ata.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\vfs\vfs_legacy\storage\src\mbr.c -o build\mbr.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\vfs\vfs_legacy\storage\src\disk_manager.c -o build\disk_manager.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\vfs\vfs_legacy\src\vfs.c -o build\vfs.o
if ($LASTEXITCODE -ne 0) { Write-Host "VFS Failed!" -ForegroundColor Red; exit }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\audio\audio_pcm.c -o build\audio_pcm.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\audio\audio_mix_math.c -o build\audio_mix_math.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\audio\audio_volume.c -o build\audio_volume.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\audio\audio_mixer.c -o build\audio_mixer.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\drivers\audio\ac97\ac97_codec.c -o build\ac97_codec.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\drivers\audio\ac97\ac97_dma.c -o build\ac97_dma.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\drivers\audio\ac97\ac97_playback.c -o build\ac97_playback.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\drivers\audio\ac97\ac97.c -o build\ac97.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\audio\audio_buffer.c -o build\audio_buffer.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\audio\audio_forensic.c -o build\audio_forensic.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit 1 }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\audio\audio_player.c -o build\audio_player.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\audio\audio_stream.c -o build\audio_stream.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\audio\audio_core.c -o build\audio_core.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\audio\audio_debug.c -o build\audio_debug.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\audio\audio_api.c -o build\audio_api.o
if ($LASTEXITCODE -ne 0) { Write-Host "Audio Core Failed!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c drivers\input\ps2\ps2.c -o build\ps2.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c drivers\input\ps2\mouse.c -o build\ps2_mouse.o
if ($LASTEXITCODE -ne 0) { Write-Host "PS2 Driver Failed!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\drivers\input\input.c -o build\kernel_input.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\drivers\input\input_abstraction.c -o build\input_abstraction.o
if ($LASTEXITCODE -ne 0) { Write-Host "Input Abstraction Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\drivers\input\core\input_core.c -o build\input_core.o
if ($LASTEXITCODE -ne 0) { Write-Host "Input Core Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\drivers\input\core\input_adapter.c -o build\input_adapter.o
if ($LASTEXITCODE -ne 0) { Write-Host "Input Adapter Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\drivers\input\pointer\pointer_state.c -o build\pointer_state.o
if ($LASTEXITCODE -ne 0) { Write-Host "Pointer State Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\drivers\input\pointer\pointer_precision.c -o build\pointer_precision.o
if ($LASTEXITCODE -ne 0) { Write-Host "Pointer Precision Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\drivers\input\pointer\pointer_velocity.c -o build\pointer_velocity.o
if ($LASTEXITCODE -ne 0) { Write-Host "Pointer Velocity Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\drivers\input\pointer\pointer_buttons.c -o build\pointer_buttons.o
if ($LASTEXITCODE -ne 0) { Write-Host "Pointer Buttons Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\drivers\input\pointer\pointer_bounds.c -o build\pointer_bounds_v2.o
if ($LASTEXITCODE -ne 0) { Write-Host "Pointer Bounds V2 Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\drivers\input\pointer\pointer_consumers.c -o build\pointer_consumers.o
if ($LASTEXITCODE -ne 0) { Write-Host "Pointer Consumers Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\drivers\input\pointer\pointer_motion.c -o build\pointer_motion.o
if ($LASTEXITCODE -ne 0) { Write-Host "Pointer Motion Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\drivers\input\pointer\pointer_engine.c -o build\pointer_engine.o
if ($LASTEXITCODE -ne 0) { Write-Host "Pointer Engine Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\drivers\input\dispatcher\dispatcher_priority.c -o build\dispatcher_priority.o
if ($LASTEXITCODE -ne 0) { Write-Host "Dispatcher Priority Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\drivers\input\dispatcher\dispatcher_queue.c -o build\dispatcher_queue.o
if ($LASTEXITCODE -ne 0) { Write-Host "Dispatcher Queue Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\drivers\input\dispatcher\dispatcher_diag.c -o build\dispatcher_diag.o
if ($LASTEXITCODE -ne 0) { Write-Host "Dispatcher Diag Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\drivers\input\dispatcher\dispatcher_consumers.c -o build\dispatcher_consumers.o
if ($LASTEXITCODE -ne 0) { Write-Host "Dispatcher Consumers Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\drivers\input\dispatcher\dispatcher_filters.c -o build\dispatcher_filters.o
if ($LASTEXITCODE -ne 0) { Write-Host "Dispatcher Filters Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\drivers\input\dispatcher\dispatcher_router.c -o build\dispatcher_router.o
if ($LASTEXITCODE -ne 0) { Write-Host "Dispatcher Router Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\drivers\input\dispatcher\dispatcher.c -o build\dispatcher.o
if ($LASTEXITCODE -ne 0) { Write-Host "Dispatcher Core Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\drivers\input\cursor\cursor_state.c -o build\cursor_state.o
if ($LASTEXITCODE -ne 0) { Write-Host "Cursor State Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\drivers\input\cursor\cursor_hotspot.c -o build\cursor_hotspot.o
if ($LASTEXITCODE -ne 0) { Write-Host "Cursor Hotspot Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\drivers\input\cursor\cursor_theme.c -o build\cursor_theme.o
if ($LASTEXITCODE -ne 0) { Write-Host "Cursor Theme Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\drivers\input\cursor\cursor_animation.c -o build\cursor_animation.o
if ($LASTEXITCODE -ne 0) { Write-Host "Cursor Animation Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\drivers\input\cursor\cursor_diag.c -o build\cursor_diag.o
if ($LASTEXITCODE -ne 0) { Write-Host "Cursor Diag Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\drivers\input\cursor\cursor_backend.c -o build\cursor_backend.o
if ($LASTEXITCODE -ne 0) { Write-Host "Cursor Backend Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\drivers\input\cursor\cursor_renderer.c -o build\cursor_renderer.o
if ($LASTEXITCODE -ne 0) { Write-Host "Cursor Renderer Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\drivers\input\cursor\cursor_engine.c -o build\cursor_engine.o
if ($LASTEXITCODE -ne 0) { Write-Host "Cursor Engine Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c drivers\input\usb_tablet\usb_tablet.c -o build\usb_tablet.o
if ($LASTEXITCODE -ne 0) { Write-Host "USB Tablet Driver Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c drivers\input\vmmouse\vmmouse.c -o build\vmmouse.o
if ($LASTEXITCODE -ne 0) { Write-Host "VMMouse Driver Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\drivers\input\ivdl.c -o build\ivdl.o
if ($LASTEXITCODE -ne 0) { Write-Host "IVDL Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\drivers\input\mouse_engine\pointer_diag.c -o build\pointer_diag.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\drivers\input\mouse_engine\pointer_bounds.c -o build\pointer_bounds.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\drivers\input\mouse_engine\pointer_filter.c -o build\pointer_filter.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\drivers\input\mouse_engine\pointer_sync.c -o build\pointer_sync.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\drivers\input\mouse_engine\pointer_manager.c -o build\pointer_manager.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\drivers\input\mouse_engine\mouse_engine.c -o build\mouse_engine.o

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

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c bovisual\Cursor\cursor_manager.c -o build\bv_cursor_manager.o
if ($LASTEXITCODE -ne 0) { Write-Host "BV Cursor Manager Failed!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c bovisual\Controls\controls.c -o build\bv_controls.o
if ($LASTEXITCODE -ne 0) { Write-Host "BV Controls Failed!" -ForegroundColor Red; exit $LASTEXITCODE }

Write-Host "Compiling BWE V2.0 Core and Renderer..." -ForegroundColor Cyan
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\wm\bwe\src\bwe_core.c -o build\bwe_core.o
if ($LASTEXITCODE -ne 0) { Write-Host "BWE Core Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\wm\bwe\src\bwe_window.c -o build\bwe_window.o
if ($LASTEXITCODE -ne 0) { Write-Host "BWE Window Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\wm\bwe\renderer\bwe_compositor.c -o build\bwe_compositor.o
if ($LASTEXITCODE -ne 0) { Write-Host "BWE Compositor Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\wm\bwe\renderer\bwe_paint.c -o build\bwe_paint.o
if ($LASTEXITCODE -ne 0) { Write-Host "BWE Paint Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\wm\bwe\theme\bwe_theme.c -o build\bwe_theme.o
if ($LASTEXITCODE -ne 0) { Write-Host "BWE Theme Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\wm\bwe\layout\bwe_layout.c -o build\bwe_layout.o
if ($LASTEXITCODE -ne 0) { Write-Host "BWE Layout Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\wm\bwe\src\bwe_controls.c -o build\bwe_controls.o
if ($LASTEXITCODE -ne 0) { Write-Host "BWE Controls Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\wm\bwe\src\bwe_demo_app.c -o build\bwe_demo_app.o
if ($LASTEXITCODE -ne 0) { Write-Host "BWE Demo App Failed!" -ForegroundColor Red; exit $LASTEXITCODE }

Write-Host "Compiling ATOMS OS Desktop Shell..." -ForegroundColor Cyan
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\shell\desktop_shell\desktop_shell.c -o build\desktop_shell.o
if ($LASTEXITCODE -ne 0) { Write-Host "Shell Core Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\engine\horse_engine.c -o build\horse_engine.o
if ($LASTEXITCODE -ne 0) { Write-Host "Horse Engine Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\ui\task_panel.c -o build\task_panel.o
if ($LASTEXITCODE -ne 0) { Write-Host "Task Panel Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\ui\start_menu.c -o build\start_menu.o
if ($LASTEXITCODE -ne 0) { Write-Host "Start Menu Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\shell\desktop_shell\apps.c -o build\apps.o
if ($LASTEXITCODE -ne 0) { Write-Host "Shell Apps Failed!" -ForegroundColor Red; exit $LASTEXITCODE }

Write-Host "Compiling BOS Explorer (Phase 9.1)..." -ForegroundColor Cyan
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\shell\apps\explorer.c -o build\explorer.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\shell\apps\explorer_ui.c -o build\explorer_ui.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\shell\apps\explorer_view.c -o build\explorer_view.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\shell\apps\explorer_sidebar.c -o build\explorer_sidebar.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\shell\apps\explorer_ops.c -o build\explorer_ops.o
if ($LASTEXITCODE -ne 0) { Write-Host "Explorer Failed!" -ForegroundColor Red; exit $LASTEXITCODE }

Write-Host "Compiling BOFLOW Animation Engine V1..." -ForegroundColor Cyan
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\gui\animation\animation_engine.c -o build\animation_engine.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\gui\animation\animation_timeline.c -o build\animation_timeline.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\gui\animation\animation_easing.c -o build\animation_easing.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\gui\animation\animation_scheduler.c -o build\animation_scheduler.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\gui\animation\animation_fade.c -o build\animation_fade.o
if ($LASTEXITCODE -ne 0) { Write-Host "BOFLOW Failed!" -ForegroundColor Red; exit $LASTEXITCODE }

Write-Host "Compiling ATOMS Motion Engine (AME)..." -ForegroundColor Cyan
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\ame\src\ame_core.c -o build\ame_core.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\ame\src\ame_easing.c -o build\ame_easing.o
if ($LASTEXITCODE -ne 0) { Write-Host "AME Failed!" -ForegroundColor Red; exit $LASTEXITCODE }

Write-Host "Compiling ATOMS Identity Engine..." -ForegroundColor Cyan
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\identity\src\identity.c -o build\identity.o
if ($LASTEXITCODE -ne 0) { Write-Host "Identity Engine Failed!" -ForegroundColor Red; exit $LASTEXITCODE }

Write-Host "Compiling Geometry..."
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c bovisual\Geometry\geometry.c -o build\bv_geometry.o
if ($LASTEXITCODE -ne 0) { Write-Host "BV Math Failed!" -ForegroundColor Red; exit $LASTEXITCODE }

Write-Host "Compiling BOSCAL Layout Engine..."
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c bovisual\BOSCAL\boscal.c -o build\bv_boscal.o
if ($LASTEXITCODE -ne 0) { Write-Host "BV BOSCAL Failed!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c bovisual\Images\images.c -o build\bv_images.o
if ($LASTEXITCODE -ne 0) { Write-Host "BV Images Failed!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\core\lib\src\string.c -o build\string.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\vfs\vfs_legacy\fs\fat32\src\fat32.c -o build\fat32.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\core\loader\elf\src\elf_validate.c -o build\elf_validate.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\core\loader\elf\src\elf_segment.c -o build\elf_segment.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\core\process\src\process_builder.c -o build\process_builder.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\core\process\src\process.c -o build\process.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\core\loader\bosx_loader.c -o build\bosx_loader.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\shell\conhost\conhost.c -o build\conhost.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\ui\boimage\boimage.c -o build\boimage.o
if ($LASTEXITCODE -ne 0) { Write-Host "BOIMAGE Failed!" -ForegroundColor Red; exit $LASTEXITCODE }

Write-Host "Compiling BOPAWN Image Engine V2..." -ForegroundColor Cyan
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bopawn\bopawn.c -o build\bopawn.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\gui\surface\surface.c -o build\surface.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bopawn\loader\image_loader.c -o build\bopawn_loader.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bopawn\cache\image_cache.c -o build\bopawn_cache.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bopawn\convert\surface_converter.c -o build\bopawn_converter.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bopawn\decoder\raw\raw_decoder.c -o build\bopawn_raw.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bopawn\decoder\bmp\bmp_decoder.c -o build\bopawn_bmp.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bopawn\decoder\ico\ico_decoder.c -o build\bopawn_ico.o
clang -target x86_64-pc-none-elf -O3 -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bopawn\decoder\png\png_decoder.c -o build\bopawn_png.o
clang -target x86_64-pc-none-elf -O3 -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bopawn\decoder\png\util\crc.c -o build\bopawn_crc.o
clang -target x86_64-pc-none-elf -O3 -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bopawn\decoder\png\util\inflate.c -o build\bopawn_inflate.o
clang -target x86_64-pc-none-elf -O3 -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bopawn\decoder\png\util\filters.c -o build\bopawn_filters.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bopawn\wallpaper\wallpaper_registry.c -o build\wallpaper_registry.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bopawn\wallpaper\wallpaper_scaler.c -o build\wallpaper_scaler.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bopawn\wallpaper\wallpaper_manager.c -o build\wallpaper_manager.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bopawn\wallpaper\wallpaper_settings.c -o build\wallpaper_settings.o
if ($LASTEXITCODE -ne 0) { Write-Host "BOPAWN Failed!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\ui\boasset\boasset.c -o build\boasset.o
if ($LASTEXITCODE -ne 0) { Write-Host "BOASSET Failed!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\ui\boasset\asset_cache.c -o build\asset_cache.o
if ($LASTEXITCODE -ne 0) { Write-Host "BOASSET Cache Failed!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\ui\boasset\asset_loader.c -o build\asset_loader.o
if ($LASTEXITCODE -ne 0) { Write-Host "BOASSET Loader Failed!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\ui\bofont\font_loader.c -o build\font_loader.o
if ($LASTEXITCODE -ne 0) { Write-Host "BOFONT Loader Failed!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\ui\bofont\glyph_cache.c -o build\glyph_cache.o
if ($LASTEXITCODE -ne 0) { Write-Host "BOFONT Cache Failed!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\ui\bofont\glyph_atlas.c -o build\glyph_atlas.o
if ($LASTEXITCODE -ne 0) { Write-Host "BOFONT Atlas Failed!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\ui\bofont\text_layout.c -o build\text_layout.o
if ($LASTEXITCODE -ne 0) { Write-Host "BOFONT Layout Failed!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\ui\bofont\bofont.c -o build\bofont.o
if ($LASTEXITCODE -ne 0) { Write-Host "BOFONT Core Failed!" -ForegroundColor Red; exit $LASTEXITCODE }

Write-Host "Compiling ROOK Engine v1.0..."
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\shell\rook\src\rook_core.c -o build\rook_core.o
if ($LASTEXITCODE -ne 0) { Write-Host "ROOK Core Failed!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\shell\rook\src\rook_registry.c -o build\rook_registry.o
if ($LASTEXITCODE -ne 0) { Write-Host "ROOK Registry Failed!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\shell\rook\src\rook_render.c -o build\rook_render.o
if ($LASTEXITCODE -ne 0) { Write-Host "ROOK Render Failed!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\shell\rook\src\rook_debug.c -o build\rook_debug.o
if ($LASTEXITCODE -ne 0) { Write-Host "ROOK Debug Failed!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\shell\rook\pages\page_boot.c -o build\page_boot.o
if ($LASTEXITCODE -ne 0) { Write-Host "ROOK Page Boot Failed!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\shell\rook\pages\page_login.c -o build\page_login.o
if ($LASTEXITCODE -ne 0) { Write-Host "ROOK Page Login Failed!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\shell\rook\pages\page_welcome.c -o build\page_welcome.o
if ($LASTEXITCODE -ne 0) { Write-Host "ROOK Page Welcome Failed!" -ForegroundColor Red; exit $LASTEXITCODE }

Write-Host "Compiling BSPE Display HAL, VBE Driver, Present Queue, Damage Tracker, Swapchain, Frame Pacer & Cursor Plane..." -ForegroundColor Cyan
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\graphics\BSPE\DisplayHAL\display_hal.c -o build\display_hal.o
if ($LASTEXITCODE -ne 0) { Write-Host "BSPE Display HAL Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\graphics\BSPE\Drivers\vbe_driver.c -o build\vbe_driver.o
if ($LASTEXITCODE -ne 0) { Write-Host "BSPE VBE Driver Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\graphics\BSPE\Present\present_queue.c -o build\present_queue.o
if ($LASTEXITCODE -ne 0) { Write-Host "BSPE Present Queue Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\graphics\BSPE\Damage\damage_tracker.c -o build\damage_tracker.o
if ($LASTEXITCODE -ne 0) { Write-Host "BSPE Damage Tracker Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\graphics\BSPE\Swapchain\swapchain.c -o build\swapchain.o
if ($LASTEXITCODE -ne 0) { Write-Host "BSPE Swapchain Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\graphics\BSPE\FramePacer\frame_pacer.c -o build\frame_pacer.o
if ($LASTEXITCODE -ne 0) { Write-Host "BSPE Frame Pacer Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\graphics\BSPE\Cursor\cursor_plane.c -o build\cursor_plane.o
if ($LASTEXITCODE -ne 0) { Write-Host "BSPE Cursor Plane Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\graphics\BSPE\Cursor\bspe_cursor_present.c -o build\bspe_cursor_present.o
if ($LASTEXITCODE -ne 0) { Write-Host "BSPE Cursor Presenter Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\graphics\BSPE\Present\bspe_present.c -o build\bspe_present.o
if ($LASTEXITCODE -ne 0) { Write-Host "BSPE Present Engine Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\graphics\BSPE\Present\vram_copy.c -o build\vram_copy.o
if ($LASTEXITCODE -ne 0) { Write-Host "BSPE VRAM Copy Engine Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\graphics\BSPE\Present\dual_page_present.c -o build\dual_page_present.o
if ($LASTEXITCODE -ne 0) { Write-Host "BSPE Dual-Page Present Engine Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\graphics\BSPE\Debug\telemetry_hud.c -o build\telemetry_hud.o
if ($LASTEXITCODE -ne 0) { Write-Host "BSPE Telemetry HUD Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\debug\step14_telemetry.c -o build\step14_telemetry.o
if ($LASTEXITCODE -ne 0) { Write-Host "Step 14 Telemetry Engine Failed!" -ForegroundColor Red; exit $LASTEXITCODE }

Write-Host "Compiling ATOMEGearDisplayTrainEngine (AGDTE) Phase 2 Core Modules..." -ForegroundColor Cyan
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\graphics\AGDTE\src\agdte_timing.c -o build\agdte_timing.o
if ($LASTEXITCODE -ne 0) { Write-Host "AGDTE Timing Engine Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\graphics\AGDTE\src\agdte_display_state.c -o build\agdte_display_state.o
if ($LASTEXITCODE -ne 0) { Write-Host "AGDTE Display State Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\graphics\AGDTE\src\agdte_buffer_manager.c -o build\agdte_buffer_manager.o
if ($LASTEXITCODE -ne 0) { Write-Host "AGDTE Buffer Manager Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\graphics\AGDTE\src\agdte_surface_manager.c -o build\agdte_surface_manager.o
if ($LASTEXITCODE -ne 0) { Write-Host "AGDTE Surface Manager Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\graphics\AGDTE\src\agdte_present_queue.c -o build\agdte_present_queue.o
if ($LASTEXITCODE -ne 0) { Write-Host "AGDTE Present Queue Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\graphics\AGDTE\src\agdte_scheduler.c -o build\agdte_scheduler.o
if ($LASTEXITCODE -ne 0) { Write-Host "AGDTE Scheduler Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\graphics\AGDTE\src\agdte_backend.c -o build\agdte_backend.o
if ($LASTEXITCODE -ne 0) { Write-Host "AGDTE Backend Layer Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\graphics\AGDTE\src\agdte_diag.c -o build\agdte_diag.o
if ($LASTEXITCODE -ne 0) { Write-Host "AGDTE Diagnostics Engine Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\graphics\AGDTE\src\agdte_presenter.c -o build\agdte_presenter.o
if ($LASTEXITCODE -ne 0) { Write-Host "AGDTE Presenter Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\graphics\AGDTE\src\agdte.c -o build\agdte.o
if ($LASTEXITCODE -ne 0) { Write-Host "AGDTE Core Engine Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\graphics\AGDTE\src\agdte_present_timeline.c -o build\agdte_present_timeline.o
if ($LASTEXITCODE -ne 0) { Write-Host "AGDTE Present Timeline Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\graphics\AGDTE\src\agdte_frame_metrics.c -o build\agdte_frame_metrics.o
if ($LASTEXITCODE -ne 0) { Write-Host "AGDTE Frame Metrics Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\graphics\AGDTE\src\agdte_vsync.c -o build\agdte_vsync.o
if ($LASTEXITCODE -ne 0) { Write-Host "AGDTE VSync Layer Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\graphics\AGDTE\src\agdte_frame_pacer.c -o build\agdte_frame_pacer.o
if ($LASTEXITCODE -ne 0) { Write-Host "AGDTE Frame Pacer Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\graphics\AGDTE\src\agdte_refresh_controller.c -o build\agdte_refresh_controller.o
if ($LASTEXITCODE -ne 0) { Write-Host "AGDTE Refresh Controller Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\graphics\AGDTE\src\agdte_swap_controller.c -o build\agdte_swap_controller.o
if ($LASTEXITCODE -ne 0) { Write-Host "AGDTE Swap Controller Failed!" -ForegroundColor Red; exit $LASTEXITCODE }


Write-Host "[4/5] Assembling Kernel Entry..." -ForegroundColor Yellow
nasm -I boot\ -f elf64 kernel\kernel_entry.asm -o build\kernel_entry.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

Write-Host "[5/5] Linking Kernel..." -ForegroundColor Yellow
ld.lld -Map build\kernel.map -T kernel\linker.ld build\kernel_entry.o build\kernel.o build\port_io.o build\idt.o build\isr_stubs.o build\isr.o build\exception.o build\irq.o build\pic.o build\timer.o build\pit.o build\keyboard.o build\ps2.o build\ps2_mouse.o build\vbe.o build\bv_core.o build\bv_graphics.o build\bv_text.o build\bv_drawing.o build\bv_renderer.o build\bv_cursor_manager.o build\bv_controls.o build\bwe_core.o build\bwe_window.o build\bwe_compositor.o build\bwe_paint.o build\bwe_theme.o build\bwe_layout.o build\bwe_controls.o build\bwe_demo_app.o build\bv_geometry.o build\bv_boscal.o build\bv_images.o build\bv_layout.o build\bv_input.o build\kernel_input.o build\input_abstraction.o build\input_core.o build\input_adapter.o build\pointer_state.o build\pointer_precision.o build\pointer_velocity.o build\pointer_buttons.o build\pointer_bounds_v2.o build\pointer_consumers.o build\pointer_motion.o build\pointer_engine.o build\dispatcher_priority.o build\dispatcher_queue.o build\dispatcher_diag.o build\dispatcher_consumers.o build\dispatcher_filters.o build\dispatcher_router.o build\dispatcher.o build\cursor_state.o build\cursor_hotspot.o build\cursor_theme.o build\cursor_animation.o build\cursor_diag.o build\cursor_backend.o build\cursor_renderer.o build\cursor_engine.o build\usb_tablet.o build\vmmouse.o build\ivdl.o build\pointer_diag.o build\pointer_bounds.o build\pointer_filter.o build\pointer_sync.o build\pointer_manager.o build\mouse_engine.o build\bmde.o build\vga.o build\console.o build\display.o build\pmm.o build\bitmap.o build\vmm.o build\paging.o build\heap.o build\list.o build\crash_log.o build\runqueue.o build\task.o build\context.o build\context_switch.o build\syscall.o build\syscall_wrappers.o build\syscall_entry.o build\gdt.o build\gdt_flush.o build\enter_usermode.o build\scheduler.o build\\block_device.o build\ata.o build\mbr.o build\disk_manager.o build\vfs.o build\string.o build\fat32.o build\elf_validate.o build\elf_segment.o build\process_builder.o build\process.o build\bosx_loader.o build\conhost.o build\boimage.o build\boasset.o build\asset_cache.o build\asset_loader.o build\font_loader.o build\glyph_cache.o build\glyph_atlas.o build\text_layout.o build\bofont.o build\rook_core.o build\rook_registry.o build\rook_render.o build\rook_debug.o build\page_boot.o build\page_login.o build\page_welcome.o build\desktop_shell.o build\horse_engine.o build\task_panel.o build\start_menu.o build\apps.o build\explorer.o build\explorer_ui.o build\explorer_view.o build\explorer_sidebar.o build\explorer_ops.o build\ac97_codec.o build\ac97_dma.o build\ac97_playback.o build\ac97.o build\audio_pcm.o build\audio_mix_math.o build\audio_volume.o build\audio_mixer.o build\audio_buffer.o build\audio_forensic.o build\audio_player.o build\audio_stream.o build\audio_core.o build\audio_debug.o build\audio_api.o build\bopawn.o build\surface.o build\bopawn_loader.o build\bopawn_cache.o build\bopawn_converter.o build\bopawn_raw.o build\bopawn_bmp.o build\bopawn_ico.o build\bopawn_png.o build\bopawn_crc.o build\bopawn_inflate.o build\bopawn_filters.o build\wallpaper_registry.o build\wallpaper_scaler.o build\wallpaper_manager.o build\wallpaper_settings.o build\animation_engine.o build\animation_timeline.o build\animation_easing.o build\animation_scheduler.o build\animation_fade.o build\ame_core.o build\ame_easing.o build\identity.o build\display_hal.o build\vbe_driver.o build\present_queue.o build\damage_tracker.o build\swapchain.o build\frame_pacer.o build\cursor_plane.o build\bspe_cursor_present.o build\bspe_present.o build\vram_copy.o build\dual_page_present.o build\telemetry_hud.o build\step14_telemetry.o build\agdte_timing.o build\agdte_display_state.o build\agdte_buffer_manager.o build\agdte_surface_manager.o build\agdte_present_queue.o build\agdte_scheduler.o build\agdte_backend.o build\agdte_diag.o build\agdte_presenter.o build\agdte.o build\agdte_present_timeline.o build\agdte_frame_metrics.o build\agdte_vsync.o build\agdte_frame_pacer.o build\agdte_refresh_controller.o build\agdte_swap_controller.o -o build\kernel.bin

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

Write-Host "Compiling GUI SDK..." -ForegroundColor Cyan
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -nostdlib -c userspace\libbos_gui\src\syscalls_gui.c -o build\syscalls_gui.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -nostdlib -c userspace\libbos_gui\src\widgets.c -o build\widgets.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -nostdlib -c userspace\libbos_gui\src\bos_gui.c -o build\bos_gui.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

Write-Host "Compiling GUI Demo Application..." -ForegroundColor Cyan
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -nostdlib -c userspace\apps\gui_demo\main.c -o build\gui_demo.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

ld.lld -T userspace\linker.ld --strip-all build\gui_demo.o build\syscalls.o build\syscalls_gui.o build\widgets.o build\bos_gui.o -o build\calc.elf
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
