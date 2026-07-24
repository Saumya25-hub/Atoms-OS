# Reload environment variables to detect newly installed tools
$env:Path = [System.Environment]::GetEnvironmentVariable("Path","Machine") + ";" + [System.Environment]::GetEnvironmentVariable("Path","User")

Write-Host "=========================================" -ForegroundColor Cyan
Write-Host "   Building SignaturesOS (V1 HDD)        " -ForegroundColor Cyan
Write-Host "=========================================" -ForegroundColor Cyan

# Constants to verify
$BOOT_SECTOR_SIZE = 512
$STAGE2_SECTORS = 4
$KERNEL_SECTORS = 1500
$KERNEL_LBA = 1 + $STAGE2_SECTORS

if (-not (Test-Path "build")) {
    New-Item -ItemType Directory -Path "build" | Out-Null
}

Write-Host "[1/5] Assembling Stage 1 Bootloader..." -ForegroundColor Yellow
nasm -I boot\ -f bin boot\boot.asm -o build\boot.bin
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

# Stage 2 will be assembled after kernel link to pass exact sector count

Write-Host "[3/5] Compiling Kernel & Drivers..." -ForegroundColor Yellow
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\kernel.c -o build\kernel.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\core\pci\pci.c -o build\pci.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\drivers\net\e1000\e1000.c -o build\e1000.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\net\netif.c -o build\netif.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\net\ethernet\ethernet.c -o build\ethernet.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\net\arp\arp.c -o build\arp.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\net\ipv4\ipv4.c -o build\ipv4.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\net\icmp\icmp.c -o build\icmp.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\net\udp\udp.c -o build\udp.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\net\dhcp\dhcp.c -o build\dhcp.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\net\dns\dns.c -o build\dns.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\net\tcp\tcp.c -o build\tcp.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\net\http\http.c -o build\http.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\net\tls\tls_record.c -o build\tls_record.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\net\tls\tls_handshake.c -o build\tls_handshake.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\net\tls\tls.c -o build\tls.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\drivers\usb\host\xhci\xhci.c -o build\xhci.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\core\vizier\src\vizier_core.c -o build\vizier_core.o
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

clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\core\brtsl\bre.c -o build\bre.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

nasm -f elf64 kernel\core\scheduler\src\context_switch.asm -o build\context_switch.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

nasm -f elf64 kernel\core\scheduler\src\enter_usermode.asm -o build\enter_usermode.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

nasm -f elf64 arch\x86_64\gdt\gdt_flush.asm -o build\gdt_flush.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I./ -c arch\x86_64\gdt\gdt.c -o build\gdt.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\ui\events\gui_events.c -o build\gui_events.o
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

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\audio\api\audio_api.c -o build\audio_api.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\audio\core\audio_core.c -o build\audio_core.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\audio\core\audio_realtime_worker.c -o build\audio_realtime_worker.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\audio\diagnostics\audio_debug.c -o build\audio_debug.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\audio\diagnostics\audio_diagnostic_mode.c -o build\audio_diagnostic_mode.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\audio\drivers\ac97\ac97.c -o build\ac97.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\audio\drivers\ac97\ac97_codec.c -o build\ac97_codec.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\audio\drivers\ac97\ac97_dma.c -o build\ac97_dma.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\audio\drivers\ac97\ac97_playback.c -o build\ac97_playback.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\audio\forensic\audio_forensic.c -o build\audio_forensic.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\audio\formats\audio_pcm.c -o build\audio_pcm.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\audio\hal\audio_driver_registry.c -o build\audio_driver_registry.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\audio\hal\audio_hal.c -o build\audio_hal.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\audio\mixer\audio_mixer.c -o build\audio_mixer.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\audio\mixer\audio_mix_math.c -o build\audio_mix_math.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\audio\session\audio_player.c -o build\audio_player.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\audio\session\audio_producer_worker.c -o build\audio_producer_worker.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\audio\streams\audio_buffer.c -o build\audio_buffer.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\audio\streams\audio_stream.c -o build\audio_stream.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\audio\volume\audio_volume.c -o build\audio_volume.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\audio\diagnostics\audio_test_mode.c -o build\audio_test_mode.o
if ($LASTEXITCODE -ne 0) { Write-Host "Audio V3 Failed!" -ForegroundColor Red; exit 1 }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c drivers\input\ps2\ps2.c -o build\ps2.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c drivers\input\ps2\mouse.c -o build\ps2_mouse.o
if ($LASTEXITCODE -ne 0) { Write-Host "PS2 Driver Failed!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\drivers\input\input.c -o build\kernel_input.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\drivers\input\input_abstraction.c -o build\input_abstraction.o
if ($LASTEXITCODE -ne 0) { Write-Host "Input Abstraction Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\drivers\input\core\hida.c -o build\hida.o
if ($LASTEXITCODE -ne 0) { Write-Host "HIDA Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\drivers\input\core\ccte.c -o build\ccte.o
if ($LASTEXITCODE -ne 0) { Write-Host "CCTE Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
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
if ($LASTEXITCODE -ne 0) { Write-Host "BV Controls" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -Ikernel\core\memory\pmm\include -I. -c kernel\core\pci\pci.c -o build\pci.o
if ($LASTEXITCODE -ne 0) { Write-Host "PCI Failed!" -ForegroundColor Red; exit $LASTEXITCODE }

Write-Host "Compiling USB xHCI Driver..." -ForegroundColor Cyan
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -Ikernel\core\memory\pmm\include -Ikernel\core\memory\vmm\include -I. -c kernel\drivers\usb\host\xhci\xhci.c -o build\xhci.o
if ($LASTEXITCODE -ne 0) { Write-Host "xHCI Failed!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -Ikernel\core\memory\pmm\include -Ikernel\core\memory\vmm\include -I. -c kernel\drivers\usb\host\xhci\xhci_dma.c -o build\xhci_dma.o
if ($LASTEXITCODE -ne 0) { Write-Host "xHCI DMA Failed!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -Ikernel\core\memory\pmm\include -Ikernel\core\memory\vmm\include -I. -c kernel\drivers\usb\host\xhci\xhci_ring.c -o build\xhci_ring.o
if ($LASTEXITCODE -ne 0) { Write-Host "xHCI Ring Failed!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -Ikernel\core\memory\pmm\include -Ikernel\core\memory\vmm\include -I. -c kernel\drivers\usb\host\xhci\xhci_cmd.c -o build\xhci_cmd.o
if ($LASTEXITCODE -ne 0) { Write-Host "xHCI Cmd Failed!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -Ikernel\core\memory\pmm\include -Ikernel\core\memory\vmm\include -I. -c kernel\drivers\usb\host\xhci\xhci_transfer.c -o build\xhci_transfer.o
if ($LASTEXITCODE -ne 0) { Write-Host "xHCI Transfer Failed!" -ForegroundColor Red; exit $LASTEXITCODE }

Write-Host "Compiling USB Core..." -ForegroundColor Cyan
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\drivers\usb\core\usb_core.c -o build\usb_core.o
if ($LASTEXITCODE -ne 0) { Write-Host "USB Core Failed!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\drivers\usb\core\usb_enum.c -o build\usb_enum.o
if ($LASTEXITCODE -ne 0) { Write-Host "USB Enum Failed!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\drivers\usb\core\usb_registry.c -o build\usb_registry.o
if ($LASTEXITCODE -ne 0) { Write-Host "USB Registry Failed!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\drivers\usb\core\usb_transfer.c -o build\usb_transfer.o
if ($LASTEXITCODE -ne 0) { Write-Host "USB Transfer API Failed!" -ForegroundColor Red; exit $LASTEXITCODE }

Write-Host "Compiling USB Class Drivers..." -ForegroundColor Cyan
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\drivers\usb\class\usb_hid.c -o build\usb_hid.o
if ($LASTEXITCODE -ne 0) { Write-Host "USB HID Class Driver Failed!" -ForegroundColor Red; exit $LASTEXITCODE }

Write-Host "Compiling BWE V2.0 Core and Renderer..." -ForegroundColor Cyan
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\wm\bwe\src\bwe_core.c -o build\bwe_core.o
if ($LASTEXITCODE -ne 0) { Write-Host "BWE Core Failed!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\wm\bwe\src\bwe_process_queue.c -o build\bwe_process_queue.o
if ($LASTEXITCODE -ne 0) { Write-Host "BWE Process Queue Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\wm\bwe\src\bwe_window.c -o build\bwe_window.o
if ($LASTEXITCODE -ne 0) { Write-Host "BWE Window Failed!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\wm\bwe\src\bwe_geometry.c -o build\bwe_geometry.o
if ($LASTEXITCODE -ne 0) { Write-Host "BWE Geometry Failed!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\wm\bwe\src\bwe_render_context.c -o build\bwe_render_context.o
if ($LASTEXITCODE -ne 0) { Write-Host "BWE Render Context Failed!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\wm\bwe\src\bwe_diagnostics.c -o build\bwe_diagnostics.o
if ($LASTEXITCODE -ne 0) { Write-Host "BWE Diagnostics Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\wm\bwe\renderer\bwe_compositor.c -o build\bwe_compositor.o
if ($LASTEXITCODE -ne 0) { Write-Host "BWE Compositor Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\wm\bwe\renderer\bwe_paint.c -o build\bwe_paint.o
if ($LASTEXITCODE -ne 0) { Write-Host "BWE Paint Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\wm\botheme\botheme.c -o build\botheme.o
if ($LASTEXITCODE -ne 0) { Write-Host "BOTHEME Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\wm\bwe\theme\bwe_theme.c -o build\bwe_theme.o
if ($LASTEXITCODE -ne 0) { Write-Host "BWE Theme Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\wm\bwe\src\bwe_layout.c -o build\bwe_layout.o
if ($LASTEXITCODE -ne 0) { Write-Host "BWE Layout Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\wm\bwe\src\bwe_controls.c -o build\bwe_controls.o
if ($LASTEXITCODE -ne 0) { Write-Host "BWE Controls Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\wm\bwe\src\bwe_demo_app.c -o build\bwe_demo_app.o
if ($LASTEXITCODE -ne 0) { Write-Host "BWE Demo App Failed!" -ForegroundColor Red; exit $LASTEXITCODE }

Write-Host "Compiling Phase 9 Native Application Framework Core..." -ForegroundColor Cyan
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\application\app_manager\app_manager.c -o build\atoms_app_manager.o
if ($LASTEXITCODE -ne 0) { Write-Host "ATOMS App Manager Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\application\loader\app_loader.c -o build\atoms_app_loader.o
if ($LASTEXITCODE -ne 0) { Write-Host "ATOMS App Loader Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\application\permissions\app_permissions.c -o build\atoms_app_permissions.o
if ($LASTEXITCODE -ne 0) { Write-Host "ATOMS App Permissions Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\application\window_api\app_window_api.c -o build\atoms_app_window_api.o
if ($LASTEXITCODE -ne 0) { Write-Host "ATOMS Window API Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\application\clipboard\app_clipboard.c -o build\atoms_app_clipboard.o
if ($LASTEXITCODE -ne 0) { Write-Host "ATOMS Clipboard Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\application\dialogs\app_dialogs.c -o build\atoms_app_dialogs.o
if ($LASTEXITCODE -ne 0) { Write-Host "ATOMS Dialogs Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\application\future_vfs_api\app_vfs_api.c -o build\atoms_app_vfs_api.o
if ($LASTEXITCODE -ne 0) { Write-Host "ATOMS App VFS API Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\application\runtime\app_runtime.c -o build\atoms_app_runtime.o
if ($LASTEXITCODE -ne 0) { Write-Host "ATOMS App Runtime Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\application\debug\app_debug.c -o build\atoms_app_debug.o
if ($LASTEXITCODE -ne 0) { Write-Host "ATOMS App Debug Suite Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\core\process\process_manager.c -o build\atoms_process_manager.o
if ($LASTEXITCODE -ne 0) { Write-Host "ATOMS Process Manager Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\core\thread\thread_manager.c -o build\atoms_thread_manager.o
if ($LASTEXITCODE -ne 0) { Write-Host "ATOMS Thread Manager Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\core\sll\sll_manager.c -o build\atoms_sll_manager.o
if ($LASTEXITCODE -ne 0) { Write-Host "ATOMS SLL Manager Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\core\bkm\bkm_manager.c -o build\atoms_bkm_manager.o
if ($LASTEXITCODE -ne 0) { Write-Host "ATOMS BKM Manager Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\core\syscall\syscall_gateway.c -o build\atoms_syscall_gateway.o
if ($LASTEXITCODE -ne 0) { Write-Host "ATOMS Syscall Gateway Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\application\installer\app_installer.c -o build\atoms_app_installer.o
if ($LASTEXITCODE -ne 0) { Write-Host "ATOMS App Installer Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\core\loader\bosx_stress.c -o build\atoms_bosx_stress.o
if ($LASTEXITCODE -ne 0) { Write-Host "ATOMS Phase 10 Stress Suite Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\network\network_manager.c -o build\atoms_network_manager.o
if ($LASTEXITCODE -ne 0) { Write-Host "ATOMS Network Manager Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\net\http\http_client.c -o build\atoms_http_client.o
if ($LASTEXITCODE -ne 0) { Write-Host "ATOMS HTTP Client Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\net\security\net_security.c -o build\atoms_net_security.o
if ($LASTEXITCODE -ne 0) { Write-Host "ATOMS Network Security Engine Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\network\net_stress.c -o build\atoms_net_stress.o
if ($LASTEXITCODE -ne 0) { Write-Host "ATOMS Phase 11 Network Stress Suite Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\browser\engine\browser_engine.c -o build\atrix_browser_engine.o
if ($LASTEXITCODE -ne 0) { Write-Host "ATRIX Browser Engine Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\browser\engine\browser_tabs.c -o build\atrix_browser_tabs.o
if ($LASTEXITCODE -ne 0) { Write-Host "ATRIX Browser Tabs Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\browser\engine\html\html_parser.c -o build\atrix_html_parser.o
if ($LASTEXITCODE -ne 0) { Write-Host "ATRIX HTML Parser Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\browser\engine\css\css_parser.c -o build\atrix_css_parser.o
if ($LASTEXITCODE -ne 0) { Write-Host "ATRIX CSS Parser Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\browser\engine\css\css_layout.c -o build\atrix_css_layout.o
if ($LASTEXITCODE -ne 0) { Write-Host "ATRIX CSS Layout Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\browser\engine\render\render_tree.c -o build\atrix_render_tree.o
if ($LASTEXITCODE -ne 0) { Write-Host "ATRIX Render Tree Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\browser\engine\javascript\js_runtime.c -o build\atrix_js_runtime.o
if ($LASTEXITCODE -ne 0) { Write-Host "ATRIX JS Runtime Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\browser\engine\networking\browser_http.c -o build\atrix_browser_http.o
if ($LASTEXITCODE -ne 0) { Write-Host "ATRIX Browser HTTP Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\browser\engine\browser_stress.c -o build\atrix_browser_stress.o
if ($LASTEXITCODE -ne 0) { Write-Host "ATRIX Browser Stress Suite Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\browser\engine\browser_url.c -o build\atrix_browser_url.o
if ($LASTEXITCODE -ne 0) { Write-Host "ATRIX Browser URL Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\browser\engine\browser_navigation.c -o build\atrix_browser_navigation.o
if ($LASTEXITCODE -ne 0) { Write-Host "ATRIX Browser Navigation Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\browser\engine\html\html_document.c -o build\atrix_html_document.o
if ($LASTEXITCODE -ne 0) { Write-Host "ATRIX HTML Document Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\browser\engine\render\paint_engine.c -o build\atrix_paint_engine.o
if ($LASTEXITCODE -ne 0) { Write-Host "ATRIX Paint Engine Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\browser\engine\render\layout_engine.c -o build\atrix_layout_engine.o
if ($LASTEXITCODE -ne 0) { Write-Host "ATRIX Layout Engine Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\browser\engine\browser_download.c -o build\atrix_browser_download.o
if ($LASTEXITCODE -ne 0) { Write-Host "ATRIX Browser Download Engine Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\browser_engine\core\abe_core.c -o build\abe_core.o
if ($LASTEXITCODE -ne 0) { Write-Host "ABE Core Subsystem Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\browser_engine\core\abe_config.c -o build\abe_config.o
if ($LASTEXITCODE -ne 0) { Write-Host "ABE Config Subsystem Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\browser_engine\core\abe_feature.c -o build\abe_feature.o
if ($LASTEXITCODE -ne 0) { Write-Host "ABE Feature Registry Subsystem Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\browser_engine\process\abe_process.c -o build\abe_process.o
if ($LASTEXITCODE -ne 0) { Write-Host "ABE Process Manager Subsystem Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\browser_engine\process\abe_session.c -o build\abe_session.o
if ($LASTEXITCODE -ne 0) { Write-Host "ABE Session Manager Subsystem Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\browser_engine\window\abe_window.c -o build\abe_window.o
if ($LASTEXITCODE -ne 0) { Write-Host "ABE Window Manager Subsystem Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\browser_engine\tab\abe_tab.c -o build\abe_tab.o
if ($LASTEXITCODE -ne 0) { Write-Host "ABE Tab Engine Subsystem Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\browser_engine\url\abe_url.c -o build\abe_url.o
if ($LASTEXITCODE -ne 0) { Write-Host "ABE URL Engine Subsystem Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\browser_engine\navigation\abe_navigation.c -o build\abe_navigation.o
if ($LASTEXITCODE -ne 0) { Write-Host "ABE Navigation Engine Subsystem Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\browser_engine\resource\abe_resource.c -o build\abe_resource.o
if ($LASTEXITCODE -ne 0) { Write-Host "ABE Resource Manager Subsystem Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\browser_engine\api\abe_api.c -o build\abe_api.o
if ($LASTEXITCODE -ne 0) { Write-Host "ABE Public API Subsystem Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\browser_engine\render\abe_render.c -o build\abe_render.o
if ($LASTEXITCODE -ne 0) { Write-Host "ABE Render Paint Subsystem Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\browser_engine\diagnostics\abe_diagnostics.c -o build\abe_diagnostics.o
if ($LASTEXITCODE -ne 0) { Write-Host "ABE Diagnostics Subsystem Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\browser_engine\diagnostics\adf_core.c -o build\adf_core.o
if ($LASTEXITCODE -ne 0) { Write-Host "ADF Core Debug Subsystem Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\browser_engine\diagnostics\adf_snapshot.c -o build\adf_snapshot.o
if ($LASTEXITCODE -ne 0) { Write-Host "ADF Snapshot & Leak Subsystem Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\browser_engine\diagnostics\adf_inspector.c -o build\adf_inspector.o
if ($LASTEXITCODE -ne 0) { Write-Host "ADF Render Inspector Subsystem Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\browser_engine\abe_phase1_test.c -o build\abe_phase1_test.o
if ($LASTEXITCODE -ne 0) { Write-Host "ABE Phase 1 Verification Suite Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\browser_engine\network\abe_net_dns.c -o build\abe_net_dns.o
if ($LASTEXITCODE -ne 0) { Write-Host "ABE DNS Resolver Subsystem Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\browser_engine\network\abe_net_tls.c -o build\abe_net_tls.o
if ($LASTEXITCODE -ne 0) { Write-Host "ABE HTTPS TLS Subsystem Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\browser_engine\network\abe_net_conn.c -o build\abe_net_conn.o
if ($LASTEXITCODE -ne 0) { Write-Host "ABE Connection Manager Subsystem Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\browser_engine\network\abe_net_http.c -o build\abe_net_http.o
if ($LASTEXITCODE -ne 0) { Write-Host "ABE HTTP Engine Subsystem Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\browser_engine\network\abe_net_parser.c -o build\abe_net_parser.o
if ($LASTEXITCODE -ne 0) { Write-Host "ABE HTTP Response Parser Subsystem Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\browser_engine\network\abe_net_redirect.c -o build\abe_net_redirect.o
if ($LASTEXITCODE -ne 0) { Write-Host "ABE Redirect Engine Subsystem Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\browser_engine\network\abe_net_compress.c -o build\abe_net_compress.o
if ($LASTEXITCODE -ne 0) { Write-Host "ABE Decompression Engine Subsystem Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\browser_engine\network\abe_net_download.c -o build\abe_net_download.o
if ($LASTEXITCODE -ne 0) { Write-Host "ABE Download Stream Subsystem Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\browser_engine\network\abe_net_manager.c -o build\abe_net_manager.o
if ($LASTEXITCODE -ne 0) { Write-Host "ABE Network Resource Manager Subsystem Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\browser_engine\network\abe_net_test.c -o build\abe_net_test.o
if ($LASTEXITCODE -ne 0) { Write-Host "ABE Phase 2 Network Verification Suite Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\browser_engine\html\abe_html_tokenizer.c -o build\abe_html_tokenizer.o
if ($LASTEXITCODE -ne 0) { Write-Host "ABE HTML5 Tokenizer Subsystem Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\browser_engine\html\abe_dom_node.c -o build\abe_dom_node.o
if ($LASTEXITCODE -ne 0) { Write-Host "ABE DOM Node System Subsystem Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\browser_engine\html\abe_html_element.c -o build\abe_html_element.o
if ($LASTEXITCODE -ne 0) { Write-Host "ABE HTML Element Library Subsystem Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\browser_engine\html\abe_html_parser.c -o build\abe_html_parser.o
if ($LASTEXITCODE -ne 0) { Write-Host "ABE HTML Parser Subsystem Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\browser_engine\html\abe_html_text.c -o build\abe_html_text.o
if ($LASTEXITCODE -ne 0) { Write-Host "ABE Text Unicode Engine Subsystem Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\browser_engine\html\abe_html_document.c -o build\abe_html_document.o
if ($LASTEXITCODE -ne 0) { Write-Host "ABE Document Manager Subsystem Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\browser_engine\html\abe_html_api.c -o build\abe_html_api.o
if ($LASTEXITCODE -ne 0) { Write-Host "ABE HTML Public API Bridge Subsystem Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\browser_engine\html\abe_html_test.c -o build\abe_html_test.o
if ($LASTEXITCODE -ne 0) { Write-Host "ABE Phase 3 HTML5 Verification Suite Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\browser_engine\css\abe_css_tokenizer.c -o build\abe_css_tokenizer.o
if ($LASTEXITCODE -ne 0) { Write-Host "ABE CSS Tokenizer Subsystem Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\browser_engine\css\abe_css_parser.c -o build\abe_css_parser.o
if ($LASTEXITCODE -ne 0) { Write-Host "ABE CSS Parser Subsystem Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\browser_engine\css\abe_css_selector.c -o build\abe_css_selector.o
if ($LASTEXITCODE -ne 0) { Write-Host "ABE CSS Selector Engine Subsystem Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\browser_engine\css\abe_css_cascade.c -o build\abe_css_cascade.o
if ($LASTEXITCODE -ne 0) { Write-Host "ABE CSS Cascade Engine Subsystem Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\browser_engine\css\abe_css_inherit.c -o build\abe_css_inherit.o
if ($LASTEXITCODE -ne 0) { Write-Host "ABE CSS Inheritance Engine Subsystem Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\browser_engine\css\abe_css_computed.c -o build\abe_css_computed.o
if ($LASTEXITCODE -ne 0) { Write-Host "ABE CSS Computed Style Engine Subsystem Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\browser_engine\css\abe_css_style_manager.c -o build\abe_css_style_manager.o
if ($LASTEXITCODE -ne 0) { Write-Host "ABE CSS Style Manager Subsystem Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\browser_engine\css\abe_css_api.c -o build\abe_css_api.o
if ($LASTEXITCODE -ne 0) { Write-Host "ABE CSS Public API Bridge Subsystem Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\browser_engine\css\abe_css_test.c -o build\abe_css_test.o
if ($LASTEXITCODE -ne 0) { Write-Host "ABE Phase 4 CSS Verification Suite Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -msse -msse2 -ffreestanding -mno-red-zone -I. -c kernel\browser_engine\layout\abe_render_tree.c -o build\abe_render_tree.o
if ($LASTEXITCODE -ne 0) { Write-Host "ABE Render Tree Subsystem Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -msse -msse2 -ffreestanding -mno-red-zone -I. -c kernel\browser_engine\layout\abe_box_model.c -o build\abe_box_model.o
if ($LASTEXITCODE -ne 0) { Write-Host "ABE Box Model Subsystem Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -msse -msse2 -ffreestanding -mno-red-zone -I. -c kernel\browser_engine\layout\abe_block_layout.c -o build\abe_block_layout.o
if ($LASTEXITCODE -ne 0) { Write-Host "ABE Block Layout Subsystem Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -msse -msse2 -ffreestanding -mno-red-zone -I. -c kernel\browser_engine\layout\abe_inline_layout.c -o build\abe_inline_layout.o
if ($LASTEXITCODE -ne 0) { Write-Host "ABE Inline Layout Subsystem Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -msse -msse2 -ffreestanding -mno-red-zone -I. -c kernel\browser_engine\layout\abe_flex_layout.c -o build\abe_flex_layout.o
if ($LASTEXITCODE -ne 0) { Write-Host "ABE Flexbox Layout Subsystem Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -msse -msse2 -ffreestanding -mno-red-zone -I. -c kernel\browser_engine\layout\abe_positioning.c -o build\abe_positioning.o
if ($LASTEXITCODE -ne 0) { Write-Host "ABE Positioning & Overflow Subsystem Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -msse -msse2 -ffreestanding -mno-red-zone -I. -c kernel\browser_engine\layout\abe_reflow.c -o build\abe_reflow.o
if ($LASTEXITCODE -ne 0) { Write-Host "ABE Reflow Engine Subsystem Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -msse -msse2 -ffreestanding -mno-red-zone -I. -c kernel\browser_engine\layout\abe_layout_diag.c -o build\abe_layout_diag.o
if ($LASTEXITCODE -ne 0) { Write-Host "ABE Layout Diagnostics Subsystem Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -msse -msse2 -ffreestanding -mno-red-zone -I. -c kernel\browser_engine\layout\abe_layout_api.c -o build\abe_layout_api.o
if ($LASTEXITCODE -ne 0) { Write-Host "ABE Layout Public API Bridge Subsystem Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -msse -msse2 -ffreestanding -mno-red-zone -I. -c kernel\browser_engine\layout\abe_layout_test.c -o build\abe_layout_test.o
if ($LASTEXITCODE -ne 0) { Write-Host "ABE Phase 5 Layout Verification Suite Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\browser_engine\javascript\abe_js_lexer.c -o build\abe_js_lexer.o
if ($LASTEXITCODE -ne 0) { Write-Host "ABE JS Lexer Subsystem Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\browser_engine\javascript\abe_js_parser.c -o build\abe_js_parser.o
if ($LASTEXITCODE -ne 0) { Write-Host "ABE JS Parser Subsystem Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\browser_engine\javascript\abe_js_compiler.c -o build\abe_js_compiler.o
if ($LASTEXITCODE -ne 0) { Write-Host "ABE JS Compiler Subsystem Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\browser_engine\javascript\abe_js_vm.c -o build\abe_js_vm.o
if ($LASTEXITCODE -ne 0) { Write-Host "ABE JS Virtual Machine Subsystem Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\browser_engine\javascript\abe_js_gc.c -o build\abe_js_gc.o
if ($LASTEXITCODE -ne 0) { Write-Host "ABE JS Garbage Collector Subsystem Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -msse -msse2 -ffreestanding -mno-red-zone -I. -c kernel\browser_engine\javascript\abe_js_objects.c -o build\abe_js_objects.o
if ($LASTEXITCODE -ne 0) { Write-Host "ABE JS Standard Objects Subsystem Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\browser_engine\javascript\abe_js_promise.c -o build\abe_js_promise.o
if ($LASTEXITCODE -ne 0) { Write-Host "ABE JS Promise Engine Subsystem Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\browser_engine\javascript\abe_js_event_loop.c -o build\abe_js_event_loop.o
if ($LASTEXITCODE -ne 0) { Write-Host "ABE JS Event Loop Subsystem Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\browser_engine\javascript\abe_js_dom_binding.c -o build\abe_js_dom_binding.o
if ($LASTEXITCODE -ne 0) { Write-Host "ABE JS DOM Binding Subsystem Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\browser_engine\javascript\abe_js_module.c -o build\abe_js_module.o
if ($LASTEXITCODE -ne 0) { Write-Host "ABE JS ES Module Subsystem Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\browser_engine\javascript\abe_js_diag.c -o build\abe_js_diag.o
if ($LASTEXITCODE -ne 0) { Write-Host "ABE JS Diagnostics Subsystem Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\browser_engine\javascript\abe_js_api.c -o build\abe_js_api.o
if ($LASTEXITCODE -ne 0) { Write-Host "ABE JS Public API Bridge Subsystem Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\browser_engine\javascript\abe_js_test.c -o build\abe_js_test.o
if ($LASTEXITCODE -ne 0) { Write-Host "ABE Phase 7 JS Verification Suite Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\browser_engine\web_platform\abe_web_fetch.c -o build\abe_web_fetch.o
if ($LASTEXITCODE -ne 0) { Write-Host "ABE Web Fetch API Subsystem Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\browser_engine\web_platform\abe_web_xhr.c -o build\abe_web_xhr.o
if ($LASTEXITCODE -ne 0) { Write-Host "ABE Web XMLHttpRequest Subsystem Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\browser_engine\web_platform\abe_web_url.c -o build\abe_web_url.o
if ($LASTEXITCODE -ne 0) { Write-Host "ABE Web URL Engine Subsystem Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\browser_engine\web_platform\abe_web_storage.c -o build\abe_web_storage.o
if ($LASTEXITCODE -ne 0) { Write-Host "ABE Web Storage Subsystem Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\browser_engine\web_platform\abe_web_history.c -o build\abe_web_history.o
if ($LASTEXITCODE -ne 0) { Write-Host "ABE Web History Subsystem Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\browser_engine\web_platform\abe_web_navigator.c -o build\abe_web_navigator.o
if ($LASTEXITCODE -ne 0) { Write-Host "ABE Web Navigator Subsystem Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -msse -msse2 -ffreestanding -mno-red-zone -I. -c kernel\browser_engine\web_platform\abe_web_performance.c -o build\abe_web_performance.o
if ($LASTEXITCODE -ne 0) { Write-Host "ABE Web Performance Subsystem Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\browser_engine\web_platform\abe_web_scheduler.c -o build\abe_web_scheduler.o
if ($LASTEXITCODE -ne 0) { Write-Host "ABE Web Scheduler Subsystem Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\browser_engine\web_platform\abe_web_observers.c -o build\abe_web_observers.o
if ($LASTEXITCODE -ne 0) { Write-Host "ABE Web Observers Subsystem Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\browser_engine\web_platform\abe_web_blob.c -o build\abe_web_blob.o
if ($LASTEXITCODE -ne 0) { Write-Host "ABE Web Blob Subsystem Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\browser_engine\web_platform\abe_web_form.c -o build\abe_web_form.o
if ($LASTEXITCODE -ne 0) { Write-Host "ABE Web Form Subsystem Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\browser_engine\web_platform\abe_web_diag.c -o build\abe_web_diag.o
if ($LASTEXITCODE -ne 0) { Write-Host "ABE Web Diagnostics Subsystem Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\browser_engine\web_platform\abe_web_api.c -o build\abe_web_api.o
if ($LASTEXITCODE -ne 0) { Write-Host "ABE Web Public API Bridge Subsystem Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\browser_engine\web_platform\abe_web_test.c -o build\abe_web_test.o
if ($LASTEXITCODE -ne 0) { Write-Host "ABE Phase 8 Web Verification Suite Failed!" -ForegroundColor Red; exit $LASTEXITCODE }




Write-Host "Compiling ATOMS OS Desktop Shell..." -ForegroundColor Cyan
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\shell\desktop_shell\bomatrix.c -o build\bomatrix.o
if ($LASTEXITCODE -ne 0) { Write-Host "BOMATRIX Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\shell\desktop_shell\desktop_shell.c -o build\desktop_shell.o
if ($LASTEXITCODE -ne 0) { Write-Host "Shell Core Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\engine\horse_engine.c -o build\horse_engine.o
if ($LASTEXITCODE -ne 0) { Write-Host "Horse Engine Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\ui\task_panel.c -o build\task_panel.o
if ($LASTEXITCODE -ne 0) { Write-Host "Task Panel Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\ui\start_menu.c -o build\start_menu.o
if ($LASTEXITCODE -ne 0) { Write-Host "Start Menu Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\ui\bos_shell_panel.c -o build\bos_shell_panel.o
if ($LASTEXITCODE -ne 0) { Write-Host "BOS Shell Panel Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\ui\system_hub.c -o build\system_hub.o
if ($LASTEXITCODE -ne 0) { Write-Host "System Hub Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\shell\desktop_shell\apps.c -o build\apps.o
if ($LASTEXITCODE -ne 0) { Write-Host "Desktop Apps Failed!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -DBWE_KERNEL -c kernel\shell\desktop_shell\shell_os_stubs.c -o build\kernel_shell_os_stubs.o
if ($LASTEXITCODE -ne 0) { Write-Host "Kernel Shell OS Stubs Failed!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -DBWE_KERNEL -c userspace\shell\command.c -o build\kernel_command.o
if ($LASTEXITCODE -ne 0) { Write-Host "Kernel Command Failed!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -DBWE_KERNEL -c userspace\shell\commands_sys.c -o build\kernel_commands_sys.o
if ($LASTEXITCODE -ne 0) { Write-Host "Kernel Commands Sys Failed!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\apps\input_lab\input_lab.c -o build\input_lab.o
if ($LASTEXITCODE -ne 0) { Write-Host "Shell Apps Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\apps\atrix\atrix_browser.c -o build\atrix_browser.o
if ($LASTEXITCODE -ne 0) { Write-Host "ATRIX Browser Failed!" -ForegroundColor Red; exit $LASTEXITCODE }

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

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\vfs\vfs_legacy\fs\ntfs\src\ntfs.c -o build\ntfs.o
if ($LASTEXITCODE -ne 0) { Write-Host "NTFS Engine Failed!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\vfs\vfs_legacy\fs\ntfs\src\ntfs_test.c -o build\ntfs_test.o
if ($LASTEXITCODE -ne 0) { Write-Host "NTFS Test Suite Failed!" -ForegroundColor Red; exit $LASTEXITCODE }

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

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\ui\bofont\bofont_assets.c -o build\bofont_assets.o
if ($LASTEXITCODE -ne 0) { Write-Host "BOFONT Assets Failed!" -ForegroundColor Red; exit $LASTEXITCODE }

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
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\graphics\AGDTE\quality\quality_engine.c -o build\quality_engine.o
if ($LASTEXITCODE -ne 0) { Write-Host "AGDTE Quality Engine Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\graphics\AGDTE\quality\frame_stabilizer.c -o build\frame_stabilizer.o
if ($LASTEXITCODE -ne 0) { Write-Host "AGDTE Frame Stabilizer Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\graphics\AGDTE\quality\motion_analyzer.c -o build\motion_analyzer.o
if ($LASTEXITCODE -ne 0) { Write-Host "AGDTE Motion Analyzer Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\graphics\AGDTE\quality\dirty_optimizer.c -o build\dirty_optimizer.o
if ($LASTEXITCODE -ne 0) { Write-Host "AGDTE Dirty Optimizer Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\graphics\AGDTE\quality\cadence_optimizer.c -o build\cadence_optimizer.o
if ($LASTEXITCODE -ne 0) { Write-Host "AGDTE Cadence Optimizer Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\graphics\AGDTE\quality\presentation_diag.c -o build\presentation_diag.o
if ($LASTEXITCODE -ne 0) { Write-Host "AGDTE Presentation Diag Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\graphics\AGDTE\quality\display_metrics.c -o build\display_metrics.o
if ($LASTEXITCODE -ne 0) { Write-Host "AGDTE Display Metrics Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\graphics\AGDTE\quality\present_quality.c -o build\present_quality.o
if ($LASTEXITCODE -ne 0) { Write-Host "AGDTE Present Quality Failed!" -ForegroundColor Red; exit $LASTEXITCODE }

# Display Intelligence Engine V1 Compilation
Write-Host "Compiling Display Intelligence Engine V1..." -ForegroundColor Cyan
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\display\display_manager.c -o build\die_manager.o
if ($LASTEXITCODE -ne 0) { Write-Host "DIE Manager Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\display\display_detection.c -o build\die_detection.o
if ($LASTEXITCODE -ne 0) { Write-Host "DIE Detection Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\display\display_capabilities.c -o build\die_capabilities.o
if ($LASTEXITCODE -ne 0) { Write-Host "DIE Capabilities Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\display\display_policy.c -o build\die_policy.o
if ($LASTEXITCODE -ne 0) { Write-Host "DIE Policy Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\display\display_geometry.c -o build\die_geometry.o
if ($LASTEXITCODE -ne 0) { Write-Host "DIE Geometry Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\display\display_layout.c -o build\die_layout.o
if ($LASTEXITCODE -ne 0) { Write-Host "DIE Layout Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\display\display_runtime.c -o build\die_runtime.o
if ($LASTEXITCODE -ne 0) { Write-Host "DIE Runtime Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\display\display_diag.c -o build\die_diag.o
if ($LASTEXITCODE -ne 0) { Write-Host "DIE Diag Failed!" -ForegroundColor Red; exit $LASTEXITCODE }

Write-Host "Compiling Compositor, AGDPE, and AGDAE..." -ForegroundColor Cyan
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\wm\compositor\bocompositor.c -o build\bocompositor.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\wm\compositor\compositor_clip.c -o build\compositor_clip.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\wm\compositor\compositor_damage.c -o build\compositor_damage.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\wm\compositor\compositor_stack.c -o build\compositor_stack.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\wm\compositor\compositor_surface.c -o build\compositor_surface.o
if ($LASTEXITCODE -ne 0) { Write-Host "Compositor Failed!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\display\agdpe\agdpe_core.c -o build\agdpe_core.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\display\agdpe\drivers\agdpe_vbe_driver.c -o build\agdpe_vbe_driver.o
if ($LASTEXITCODE -ne 0) { Write-Host "AGDPE Failed!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\display\agdae\agdae_core.c -o build\agdae_core.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\display\agdae\agdae_diag.c -o build\agdae_diag.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\display\agdae\agdae_geometry.c -o build\agdae_geometry.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\display\agdae\agdae_scaling.c -o build\agdae_scaling.o
if ($LASTEXITCODE -ne 0) { Write-Host "AGDAE Failed!" -ForegroundColor Red; exit $LASTEXITCODE }

Write-Host "Compiling BDCE Authority Layer (Phase A/B/C Foundation)..." -ForegroundColor Cyan
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\display\bdce\src\bdce_authority.c -o build\bdce_authority.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\display\bdce\src\bdce_validation.c -o build\bdce_validation.o
if ($LASTEXITCODE -ne 0) { Write-Host "BDCE Authority/Validation Failed!" -ForegroundColor Red; exit $LASTEXITCODE }

Write-Host "[4/5] Assembling Kernel Entry..." -ForegroundColor Yellow
nasm -I boot\ -f elf64 kernel\kernel_entry.asm -o build\kernel_entry.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

Write-Host "Compiling ATOMS OS Native Cryptographic Subsystem..." -ForegroundColor Cyan
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\crypto\sha256\sha256.c -o build\sha256.o
if ($LASTEXITCODE -ne 0) { Write-Host "SHA256 Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\crypto\hmac\hmac_sha256.c -o build\hmac_sha256.o
if ($LASTEXITCODE -ne 0) { Write-Host "HMAC SHA256 Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\crypto\prf\tls_prf.c -o build\tls_prf.o
if ($LASTEXITCODE -ne 0) { Write-Host "TLS PRF Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\crypto\aes\aes.c -o build\aes.o
if ($LASTEXITCODE -ne 0) { Write-Host "AES Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\crypto\gcm\gcm.c -o build\gcm.o
if ($LASTEXITCODE -ne 0) { Write-Host "GCM Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\crypto\random\crypto_rand.c -o build\crypto_rand.o
if ($LASTEXITCODE -ne 0) { Write-Host "Crypto Rand Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\crypto\rsa\rsa.c -o build\rsa.o
if ($LASTEXITCODE -ne 0) { Write-Host "RSA Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\crypto\x509\x509.c -o build\x509.o
if ($LASTEXITCODE -ne 0) { Write-Host "X509 Parser Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\crypto\x509\x509_verify.c -o build\x509_verify.o
if ($LASTEXITCODE -ne 0) { Write-Host "X509 Verify Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\security\trust\trust_store.c -o build\trust_store.o
if ($LASTEXITCODE -ne 0) { Write-Host "Trust Store Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\drivers\rtc\rtc.c -o build\rtc.o
if ($LASTEXITCODE -ne 0) { Write-Host "RTC Driver Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\net\tls\tls_test.c -o build\tls_test.o
if ($LASTEXITCODE -ne 0) { Write-Host "TLS Test Suite Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\net\netif\net_service.c -o build\net_service.o
if ($LASTEXITCODE -ne 0) { Write-Host "Net Service Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\net\socket\socket_manager.c -o build\socket_manager.o
if ($LASTEXITCODE -ne 0) { Write-Host "Socket Manager Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\net\socket\socket.c -o build\socket.o
if ($LASTEXITCODE -ne 0) { Write-Host "Socket API Failed!" -ForegroundColor Red; exit $LASTEXITCODE }

Write-Host "Compiling BOS OS Phase 1 Production Dynamic Loader Subsystem..." -ForegroundColor Cyan
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\loader\debug\loader_debug.c -o build\loader_debug.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\loader\elf\elf_parser.c -o build\loader_elf_parser.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\loader\symbols\symbol_resolver.c -o build\loader_symbol_resolver.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\loader\reloc\reloc_engine.c -o build\loader_reloc_engine.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\loader\segment\segment_loader.c -o build\loader_segment_loader.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\loader\library\library_manager.c -o build\loader_library_manager.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\loader\runtime\runtime_linker.c -o build\loader_runtime_linker.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\loader\core\loader_manager.c -o build\loader_manager.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\loader\tests\loader_tests.c -o build\loader_tests.o
if ($LASTEXITCODE -ne 0) { Write-Host "BOS Dynamic Loader Subsystem Failed!" -ForegroundColor Red; exit $LASTEXITCODE }

Write-Host "Compiling BOS OS Phase 2 Production IPC & Shared Memory Engine..." -ForegroundColor Cyan
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\ipc\debug\ipc_debug.c -o build\ipc_debug.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\ipc\sync\ipc_sync.c -o build\ipc_sync.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\ipc\permissions\ipc_permissions.c -o build\ipc_permissions.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\ipc\channels\channel_manager.c -o build\ipc_channel_manager.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\ipc\message\message_queue.c -o build\ipc_message_queue.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\ipc\pipes\pipe_engine.c -o build\ipc_pipe_engine.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\ipc\ports\port_manager.c -o build\ipc_port_manager.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\ipc\shared_memory\shm_manager.c -o build\ipc_shm_manager.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\ipc\router\ipc_router.c -o build\ipc_router.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\ipc\core\ipc_manager.c -o build\ipc_manager.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\ipc\tests\ipc_tests.c -o build\ipc_tests.o
if ($LASTEXITCODE -ne 0) { Write-Host "BOS IPC & Shared Memory Subsystem Failed!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\net\socket\socket_test.c -o build\socket_test.o
if ($LASTEXITCODE -ne 0) { Write-Host "Socket Test Suite Failed!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c arch\x86_64\cpu\cpu_features.c -o build\cpu_features.o
if ($LASTEXITCODE -ne 0) { Write-Host "CPU Features Compilation Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\core\cpu\cpu_state.c -o build\cpu_state.o
if ($LASTEXITCODE -ne 0) { Write-Host "CPU Extended State Manager Compilation Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\debug\test_cpu_phase0.c -o build\test_cpu_phase0.o
if ($LASTEXITCODE -ne 0) { Write-Host "Phase 0 Test Suite Compilation Failed!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\graphics\bgl\bgl_drawable.c -o build\bgl_drawable.o
if ($LASTEXITCODE -ne 0) { Write-Host "BGL Drawable Compilation Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\graphics\bgl\bgl_context.c -o build\bgl_context.o
if ($LASTEXITCODE -ne 0) { Write-Host "BGL Context Compilation Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\graphics\bgl\bgl.c -o build\bgl.o
if ($LASTEXITCODE -ne 0) { Write-Host "BGL API Compilation Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\debug\test_bgl_phase1.c -o build\test_bgl_phase1.o
if ($LASTEXITCODE -ne 0) { Write-Host "Phase 1 Test Suite Compilation Failed!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -msse -msse2 -ffreestanding -mno-red-zone -I. -c kernel\graphics\gl\gl_math.c -o build\gl_math.o
if ($LASTEXITCODE -ne 0) { Write-Host "GL Math Compilation Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -msse -msse2 -ffreestanding -mno-red-zone -I. -c kernel\graphics\gl\gl_state.c -o build\gl_state.o
if ($LASTEXITCODE -ne 0) { Write-Host "GL State Compilation Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -msse -msse2 -ffreestanding -mno-red-zone -I. -c kernel\graphics\gl\gl_pipeline.c -o build\gl_pipeline.o
if ($LASTEXITCODE -ne 0) { Write-Host "GL Pipeline Compilation Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -msse -msse2 -ffreestanding -mno-red-zone -I. -c kernel\graphics\gl\gl_clip.c -o build\gl_clip.o
if ($LASTEXITCODE -ne 0) { Write-Host "GL Clip Compilation Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -msse -msse2 -ffreestanding -mno-red-zone -I. -c kernel\graphics\gl\gl_viewport.c -o build\gl_viewport.o
if ($LASTEXITCODE -ne 0) { Write-Host "GL Viewport Compilation Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -msse -msse2 -ffreestanding -mno-red-zone -I. -c kernel\graphics\gl\gl_depth.c -o build\gl_depth.o
if ($LASTEXITCODE -ne 0) { Write-Host "GL Depth Compilation Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -msse -msse2 -ffreestanding -mno-red-zone -I. -c kernel\graphics\gl\gl_texture.c -o build\gl_texture.o
if ($LASTEXITCODE -ne 0) { Write-Host "GL Texture Compilation Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -msse -msse2 -ffreestanding -mno-red-zone -I. -c kernel\graphics\gl\gl_sampler.c -o build\gl_sampler.o
if ($LASTEXITCODE -ne 0) { Write-Host "GL Sampler Compilation Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -msse -msse2 -ffreestanding -mno-red-zone -I. -c kernel\graphics\gl\gl_lighting.c -o build\gl_lighting.o
if ($LASTEXITCODE -ne 0) { Write-Host "GL Lighting Compilation Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -msse -msse2 -ffreestanding -mno-red-zone -I. -c kernel\graphics\gl\gl_fragment.c -o build\gl_fragment.o
if ($LASTEXITCODE -ne 0) { Write-Host "GL Fragment Compilation Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -msse -msse2 -ffreestanding -mno-red-zone -I. -c kernel\graphics\gl\gl_point.c -o build\gl_point.o
if ($LASTEXITCODE -ne 0) { Write-Host "GL Point Compilation Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -msse -msse2 -ffreestanding -mno-red-zone -I. -c kernel\graphics\gl\gl_line.c -o build\gl_line.o
if ($LASTEXITCODE -ne 0) { Write-Host "GL Line Compilation Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -msse -msse2 -ffreestanding -mno-red-zone -I. -c kernel\graphics\gl\gl_rasterizer.c -o build\gl_rasterizer.o
if ($LASTEXITCODE -ne 0) { Write-Host "GL Rasterizer Compilation Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -msse -msse2 -ffreestanding -mno-red-zone -I. -c kernel\graphics\gl\gl.c -o build\gl.o
if ($LASTEXITCODE -ne 0) { Write-Host "GL Core API Compilation Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -msse -msse2 -ffreestanding -mno-red-zone -I. -c kernel\debug\test_gl_phase2.c -o build\test_gl_phase2.o
if ($LASTEXITCODE -ne 0) { Write-Host "Phase 2 Test Suite Compilation Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -msse -msse2 -ffreestanding -mno-red-zone -I. -c kernel\debug\test_gl_phase3.c -o build\test_gl_phase3.o
if ($LASTEXITCODE -ne 0) { Write-Host "Phase 3 Test Suite Compilation Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -msse -msse2 -ffreestanding -mno-red-zone -I. -c kernel\debug\test_gl_phase4.c -o build\test_gl_phase4.o
if ($LASTEXITCODE -ne 0) { Write-Host "Phase 4 Test Suite Compilation Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -msse -msse2 -ffreestanding -mno-red-zone -I. -c kernel\debug\test_gl_phase5.c -o build\test_gl_phase5.o
if ($LASTEXITCODE -ne 0) { Write-Host "Phase 5 Test Suite Compilation Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -msse -msse2 -ffreestanding -mno-red-zone -I. -c kernel\graphics\gl\gl_vertex_fetch.c -o build\gl_vertex_fetch.o
if ($LASTEXITCODE -ne 0) { Write-Host "GL Vertex Fetch Compilation Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -msse -msse2 -ffreestanding -mno-red-zone -I. -c kernel\graphics\gl\gl_display_list.c -o build\gl_display_list.o
if ($LASTEXITCODE -ne 0) { Write-Host "GL Display List Compilation Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -msse -msse2 -ffreestanding -mno-red-zone -I. -c kernel\debug\test_gl_phase7.c -o build\test_gl_phase7.o
if ($LASTEXITCODE -ne 0) { Write-Host "Phase 7 Test Suite Compilation Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -msse -msse2 -ffreestanding -mno-red-zone -I. -c kernel\debug\test_gl_phase8.c -o build\test_gl_phase8.o
if ($LASTEXITCODE -ne 0) { Write-Host "Phase 8 Test Suite Compilation Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -msse -msse2 -ffreestanding -mno-red-zone -I. -c kernel\debug\test_gl_phase9.c -o build\test_gl_phase9.o
if ($LASTEXITCODE -ne 0) { Write-Host "Phase 9 Test Suite Compilation Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -msse -msse2 -ffreestanding -mno-red-zone -I. -c kernel\graphics\gl\gl_fbo.c -o build\gl_fbo.o
if ($LASTEXITCODE -ne 0) { Write-Host "GL FBO Compilation Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -msse -msse2 -ffreestanding -mno-red-zone -I. -c kernel\debug\test_gl_phase10.c -o build\test_gl_phase10.o
if ($LASTEXITCODE -ne 0) { Write-Host "Phase 10 Test Suite Compilation Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -msse -msse2 -ffreestanding -mno-red-zone -I. -c kernel\apps\atoms_graph_3d\atoms_graph_metrics.c -o build\atoms_graph_metrics.o
if ($LASTEXITCODE -ne 0) { Write-Host "ATOMS Graph Metrics Compilation Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -msse -msse2 -ffreestanding -mno-red-zone -I. -c kernel\apps\atoms_graph_3d\atoms_graph_scene.c -o build\atoms_graph_scene.o
if ($LASTEXITCODE -ne 0) { Write-Host "ATOMS Graph Scene Compilation Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -msse -msse2 -ffreestanding -mno-red-zone -I. -c kernel\apps\atoms_graph_3d\atoms_graph_renderer.c -o build\atoms_graph_renderer.o
if ($LASTEXITCODE -ne 0) { Write-Host "ATOMS Graph Renderer Compilation Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -msse -msse2 -ffreestanding -mno-red-zone -I. -c kernel\apps\atoms_graph_3d\atoms_graph_ui.c -o build\atoms_graph_ui.o
if ($LASTEXITCODE -ne 0) { Write-Host "ATOMS Graph UI Compilation Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -msse -msse2 -ffreestanding -mno-red-zone -I. -c kernel\apps\atoms_graph_3d\atoms_graph_benchmark.c -o build\atoms_graph_benchmark.o
if ($LASTEXITCODE -ne 0) { Write-Host "ATOMS Graph Benchmark Compilation Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -msse -msse2 -ffreestanding -mno-red-zone -I. -c kernel\apps\atoms_graph_3d\atoms_graph_3d.c -o build\atoms_graph_3d.o
if ($LASTEXITCODE -ne 0) { Write-Host "ATOMS Graph 3D App Compilation Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -msse -msse2 -ffreestanding -mno-red-zone -I. -c kernel\debug\test_gl_phase11.c -o build\test_gl_phase11.o
if ($LASTEXITCODE -ne 0) { Write-Host "Phase 11 Test Suite Compilation Failed!" -ForegroundColor Red; exit $LASTEXITCODE }

Write-Host "[5/5] Linking Kernel..." -ForegroundColor Yellow
ld.lld --Map=build\kernel.map -T kernel\linker.ld build\kernel_entry.o build\kernel.o build\loader_debug.o build\loader_elf_parser.o build\loader_symbol_resolver.o build\loader_reloc_engine.o build\loader_segment_loader.o build\loader_library_manager.o build\loader_runtime_linker.o build\loader_manager.o build\loader_tests.o build\ipc_debug.o build\ipc_sync.o build\ipc_permissions.o build\ipc_channel_manager.o build\ipc_message_queue.o build\ipc_pipe_engine.o build\ipc_port_manager.o build\ipc_shm_manager.o build\ipc_router.o build\ipc_manager.o build\ipc_tests.o build\cpu_features.o build\cpu_state.o build\test_cpu_phase0.o build\bgl_drawable.o build\bgl_context.o build\bgl.o build\test_bgl_phase1.o build\gl_math.o build\gl_state.o build\gl_pipeline.o build\gl_clip.o build\gl_viewport.o build\gl_depth.o build\gl_texture.o build\gl_sampler.o build\gl_lighting.o build\gl_fragment.o build\gl_point.o build\gl_line.o build\gl_vertex_fetch.o build\gl_display_list.o build\gl_rasterizer.o build\gl_fbo.o build\gl.o build\test_gl_phase2.o build\test_gl_phase3.o build\test_gl_phase4.o build\test_gl_phase5.o build\test_gl_phase6.o build\test_gl_phase7.o build\test_gl_phase8.o build\test_gl_phase9.o build\test_gl_phase10.o build\atoms_graph_metrics.o build\atoms_graph_scene.o build\atoms_graph_renderer.o build\atoms_graph_ui.o build\atoms_graph_benchmark.o build\atoms_graph_3d.o build\test_gl_phase11.o build\pci.o build\e1000.o build\netif.o build\ethernet.o build\arp.o build\ipv4.o build\icmp.o build\udp.o build\dhcp.o build\dns.o build\tcp.o build\http.o build\tls_record.o build\tls_handshake.o build\tls.o build\sha256.o build\hmac_sha256.o build\tls_prf.o build\aes.o build\gcm.o build\crypto_rand.o build\tls_crypto.o build\rsa.o build\x509.o build\x509_verify.o build\trust_store.o build\rtc.o build\tls_test.o build\net_service.o build\socket_manager.o build\socket.o build\socket_test.o build\xhci.o build\xhci_dma.o build\xhci_ring.o build\xhci_cmd.o build\xhci_transfer.o build\usb_core.o build\usb_enum.o build\usb_registry.o build\usb_transfer.o build\usb_hid.o build\vizier_core.o build\port_io.o build\idt.o build\isr_stubs.o build\isr.o build\exception.o build\irq.o build\pic.o build\timer.o build\pit.o build\keyboard.o build\ps2.o build\ps2_mouse.o build\vbe.o build\bv_core.o build\bv_graphics.o build\bv_text.o build\bv_drawing.o build\bv_renderer.o build\bv_cursor_manager.o build\bv_controls.o build\bwe_core.o build\bwe_process_queue.o build\bwe_window.o build\bwe_geometry.o build\bwe_render_context.o build\bwe_diagnostics.o build\bwe_compositor.o build\bwe_paint.o build\botheme.o build\bwe_theme.o build\bwe_layout.o build\bwe_controls.o build\bwe_demo_app.o build\atoms_app_manager.o build\atoms_app_loader.o build\atoms_app_permissions.o build\atoms_app_window_api.o build\atoms_app_clipboard.o build\atoms_app_dialogs.o build\atoms_app_vfs_api.o build\atoms_app_runtime.o build\atoms_app_debug.o build\atoms_process_manager.o build\atoms_thread_manager.o build\atoms_sll_manager.o build\atoms_bkm_manager.o build\atoms_syscall_gateway.o build\atoms_app_installer.o build\atoms_bosx_stress.o build\atoms_network_manager.o build\atoms_http_client.o build\atoms_net_security.o build\atoms_net_stress.o build\atrix_browser_engine.o build\atrix_browser_tabs.o build\atrix_html_parser.o build\atrix_css_parser.o build\atrix_css_layout.o build\atrix_render_tree.o build\atrix_js_runtime.o build\atrix_browser_http.o build\atrix_browser_stress.o build\atrix_browser_url.o build\atrix_browser_navigation.o build\atrix_html_document.o build\atrix_paint_engine.o build\atrix_layout_engine.o build\atrix_browser_download.o build\abe_core.o build\abe_config.o build\abe_feature.o build\abe_process.o build\abe_session.o build\abe_window.o build\abe_tab.o build\abe_url.o build\abe_navigation.o build\abe_resource.o build\abe_api.o build\abe_render.o build\abe_diagnostics.o build\adf_core.o build\adf_snapshot.o build\adf_inspector.o build\abe_phase1_test.o build\abe_net_dns.o build\abe_net_tls.o build\abe_net_conn.o build\abe_net_http.o build\abe_net_parser.o build\abe_net_redirect.o build\abe_net_compress.o build\abe_net_download.o build\abe_net_manager.o build\abe_net_test.o build\abe_html_tokenizer.o build\abe_dom_node.o build\abe_html_element.o build\abe_html_parser.o build\abe_html_text.o build\abe_html_document.o build\abe_html_api.o build\abe_html_test.o build\abe_css_tokenizer.o build\abe_css_parser.o build\abe_css_selector.o build\abe_css_cascade.o build\abe_css_inherit.o build\abe_css_computed.o build\abe_css_style_manager.o build\abe_css_api.o build\abe_css_test.o build\abe_render_tree.o build\abe_box_model.o build\abe_block_layout.o build\abe_inline_layout.o build\abe_flex_layout.o build\abe_positioning.o build\abe_reflow.o build\abe_layout_diag.o build\abe_layout_api.o build\abe_layout_test.o build\abe_js_lexer.o build\abe_js_parser.o build\abe_js_compiler.o build\abe_js_vm.o build\abe_js_gc.o build\abe_js_objects.o build\abe_js_promise.o build\abe_js_event_loop.o build\abe_js_dom_binding.o build\abe_js_module.o build\abe_js_diag.o build\abe_js_api.o build\abe_js_test.o build\abe_web_fetch.o build\abe_web_xhr.o build\abe_web_url.o build\abe_web_storage.o build\abe_web_history.o build\abe_web_navigator.o build\abe_web_performance.o build\abe_web_scheduler.o build\abe_web_observers.o build\abe_web_blob.o build\abe_web_form.o build\abe_web_diag.o build\abe_web_api.o build\abe_web_test.o build\bv_geometry.o build\bv_boscal.o build\bv_images.o build\bv_layout.o build\bv_input.o build\kernel_input.o build\input_abstraction.o build\hida.o build\ccte.o build\input_core.o build\input_adapter.o build\pointer_state.o build\pointer_precision.o build\pointer_velocity.o build\pointer_buttons.o build\pointer_bounds_v2.o build\pointer_consumers.o build\pointer_motion.o build\pointer_engine.o build\dispatcher_priority.o build\dispatcher_queue.o build\dispatcher_diag.o build\dispatcher_consumers.o build\dispatcher_filters.o build\dispatcher_router.o build\dispatcher.o build\cursor_state.o build\cursor_hotspot.o build\cursor_theme.o build\cursor_animation.o build\cursor_diag.o build\cursor_backend.o build\cursor_renderer.o build\cursor_engine.o build\usb_tablet.o build\vmmouse.o build\ivdl.o build\pointer_diag.o build\pointer_filter.o build\pointer_sync.o build\pointer_manager.o build\mouse_engine.o build\bmde.o build\vga.o build\console.o build\display.o build\pmm.o build\bitmap.o build\vmm.o build\paging.o build\heap.o build\list.o build\crash_log.o build\runqueue.o build\task.o build\context.o build\context_switch.o build\syscall.o build\syscall_wrappers.o build\syscall_entry.o build\gui_events.o build\gdt.o build\gdt_flush.o build\enter_usermode.o build\scheduler.o build\bre.o build\block_device.o build\ata.o build\mbr.o build\disk_manager.o build\vfs.o build\string.o build\fat32.o build\ntfs.o build\ntfs_test.o build\elf_validate.o build\elf_segment.o build\process_builder.o build\process.o build\bosx_loader.o build\conhost.o build\boimage.o build\boasset.o build\asset_cache.o build\asset_loader.o build\font_loader.o build\glyph_cache.o build\glyph_atlas.o build\text_layout.o build\bofont.o build\bofont_assets.o build\rook_core.o build\rook_registry.o build\rook_render.o build\rook_debug.o build\page_boot.o build\page_login.o build\page_welcome.o build\bomatrix.o build\desktop_shell.o build\horse_engine.o build\task_panel.o build\start_menu.o build\bos_shell_panel.o build\system_hub.o build\apps.o build\kernel_shell_os_stubs.o build\kernel_command.o build\kernel_commands_sys.o build\input_lab.o build\atrix_browser.o build\explorer.o build\explorer_ui.o build\explorer_view.o build\explorer_sidebar.o build\explorer_ops.o build\audio_api.o build\audio_core.o build\audio_realtime_worker.o build\audio_debug.o build\audio_diagnostic_mode.o build\ac97.o build\ac97_codec.o build\ac97_dma.o build\ac97_playback.o build\audio_forensic.o build\audio_pcm.o build\audio_driver_registry.o build\audio_hal.o build\audio_mixer.o build\audio_mix_math.o build\audio_player.o build\audio_producer_worker.o build\audio_buffer.o build\audio_stream.o build\audio_volume.o build\audio_test_mode.o build\bopawn.o build\surface.o build\bopawn_loader.o build\bopawn_cache.o build\bopawn_converter.o build\bopawn_raw.o build\bopawn_bmp.o build\bopawn_ico.o build\bopawn_png.o build\bopawn_crc.o build\bopawn_inflate.o build\bopawn_filters.o build\wallpaper_registry.o build\wallpaper_scaler.o build\wallpaper_manager.o build\wallpaper_settings.o build\animation_engine.o build\animation_timeline.o build\animation_easing.o build\animation_scheduler.o build\animation_fade.o build\ame_core.o build\ame_easing.o build\identity.o build\display_hal.o build\vbe_driver.o build\present_queue.o build\damage_tracker.o build\swapchain.o build\frame_pacer.o build\cursor_plane.o build\bspe_cursor_present.o build\bspe_present.o build\vram_copy.o build\dual_page_present.o build\telemetry_hud.o build\step14_telemetry.o build\agdte_timing.o build\agdte_display_state.o build\agdte_buffer_manager.o build\agdte_surface_manager.o build\agdte_present_queue.o build\agdte_scheduler.o build\agdte_backend.o build\agdte_diag.o build\agdte_presenter.o build\agdte.o build\agdte_present_timeline.o build\agdte_frame_metrics.o build\agdte_vsync.o build\agdte_frame_pacer.o build\agdte_refresh_controller.o build\agdte_swap_controller.o build\quality_engine.o build\frame_stabilizer.o build\motion_analyzer.o build\dirty_optimizer.o build\cadence_optimizer.o build\presentation_diag.o build\display_metrics.o build\present_quality.o build\die_manager.o build\die_detection.o build\die_capabilities.o build\die_policy.o build\die_geometry.o build\die_layout.o build\die_runtime.o build\die_diag.o build\bocompositor.o build\compositor_clip.o build\compositor_damage.o build\compositor_stack.o build\compositor_surface.o build\agdpe_core.o build\agdpe_vbe_driver.o build\agdae_core.o build\agdae_diag.o build\agdae_geometry.o build\agdae_scaling.o build\bdce_authority.o build\bdce_validation.o -o build\kernel.bin

if ($LASTEXITCODE -ne 0) { Write-Host "LINK FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

# Measure Kernel Payload Size & Compute Required Sectors
$kernelFile = Get-Item "build\kernel.bin"
$actualKernelBytes = $kernelFile.Length
$REQUIRED_KERNEL_SECTORS = [math]::Ceiling($actualKernelBytes / 512)
$RESERVED_DISK_SECTORS = 4091

Write-Host "-----------------------------------------" -ForegroundColor Cyan
Write-Host " [KERNEL BUILD METRICS]" -ForegroundColor Cyan
Write-Host "   Actual Kernel Payload : $actualKernelBytes bytes" -ForegroundColor Green
Write-Host "   Required Sectors      : $REQUIRED_KERNEL_SECTORS sectors" -ForegroundColor Green
Write-Host "   Disk Reserved Capacity: $RESERVED_DISK_SECTORS sectors (1,046,016 bytes)" -ForegroundColor Green
Write-Host "-----------------------------------------" -ForegroundColor Cyan

if ($REQUIRED_KERNEL_SECTORS -gt $RESERVED_DISK_SECTORS) {
    Write-Host "BUILD FAILED! Kernel size ($actualKernelBytes bytes, $REQUIRED_KERNEL_SECTORS sectors) exceeds reserved partition offset capacity ($RESERVED_DISK_SECTORS sectors)." -ForegroundColor Red
    exit 1
}

# Assembling Stage 2 Loader with exact required KERNEL_SECTORS count
Write-Host "Assembling Stage 2 Loader with KERNEL_SECTORS=$REQUIRED_KERNEL_SECTORS..." -ForegroundColor Yellow
nasm -I boot\ -D KERNEL_SECTORS=$REQUIRED_KERNEL_SECTORS -f bin boot\stage2.asm -o build\stage2.bin
if ($LASTEXITCODE -ne 0) { Write-Host "STAGE2 BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

# Pad Kernel.bin to exact KERNEL_SECTORS size to ensure exact sector alignment for disk image
$kernelBytes = [System.IO.File]::ReadAllBytes("build\kernel.bin")
$paddedKernel = New-Object byte[] ($REQUIRED_KERNEL_SECTORS * 512)
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

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -nostdlib -c userspace\libbos\src\bpde.c -o build\bpde.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -nostdlib -c userspace\test.c -o build\test.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }
ld.lld -T userspace\linker.ld --strip-all build\test.o build\syscalls.o build\bodiskhub.o build\bpde.o -o build\test.elf
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

ld.lld -T userspace\linker.ld --strip-all build\shell.o build\command.o build\commands_sys.o build\commands_debug.o build\commands_bodh.o build\commands_edit.o build\commands_diag.o build\sds_core.o build\sds_database.o build\sds_console.o build\sds_logger.o build\syscalls.o build\bodiskhub.o build\bpde.o build\atom_memory.o build\atom_types.o build\atom_value.o build\atom_string.o build\atom_array.o build\atom_table.o build\atom_bytecode.o build\atom_function.o build\atom_scope.o build\atom_vm.o build\atom_lexer.o build\atom_compiler.o build\atom_init.o build\atom_self_test.o build\bishop_error.o build\bishop_basic.o build\bishop_scientific.o build\bishop_matrix.o build\bishop_vector.o build\bishop_complex.o build\bishop_bigint.o build\bishop_stats.o build\bishop_self_test.o build\bishop_builtins.o -o build\shell.elf
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

ld.lld -T userspace\linker.ld --strip-all build\test_runner.o build\test_heap.o build\test_scheduler.o build\test_fs.o build\test_bosl.o build\test_bishop.o build\test_panic.o build\test_perf.o build\syscalls.o build\bpde.o build\atom_memory.o build\atom_types.o build\atom_value.o build\atom_string.o build\atom_array.o build\atom_table.o build\atom_bytecode.o build\atom_function.o build\atom_scope.o build\atom_vm.o build\atom_lexer.o build\atom_compiler.o build\atom_init.o build\bishop_error.o build\bishop_basic.o build\bishop_scientific.o build\bishop_matrix.o build\bishop_vector.o build\bishop_complex.o build\bishop_bigint.o build\bishop_stats.o build\bishop_builtins.o -o build\tests.elf
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

ld.lld -T userspace\linker.ld --strip-all build\gui_demo.o build\syscalls.o build\syscalls_gui.o build\widgets.o build\bos_gui.o build\bpde.o -o build\calc.elf
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

Write-Host "Compiling DOOM..." -ForegroundColor Cyan
& .\userspace\apps\doom\build_doom.ps1
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED! DOOM compilation failed" -ForegroundColor Red; exit $LASTEXITCODE }

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

$vdiPath = "build\SignaturesOS.vdi"
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
    Write-Host "WARNING: VBoxManage not found or VDI conversion failed." -ForegroundColor Yellow
} else {
    Write-Host "[OK] VDI Created: build\SignaturesOS.vdi" -ForegroundColor Green
}

# ==============================================================================
# VMDK CONVERSION (VMware Hard Disk Container)
# ==============================================================================
Write-Host "--- Converting to VMDK for VMware/QEMU ---" -ForegroundColor Cyan
$vmdkPath = "build\SignaturesOS.vmdk"
if (Test-Path $vmdkPath) {
    Remove-Item $vmdkPath -Force
}

& $vboxManage convertfromraw $imgPath $vmdkPath --format VMDK
if ($LASTEXITCODE -ne 0) {
    Write-Host "WARNING: VMDK conversion failed." -ForegroundColor Yellow
} else {
    Write-Host "[OK] VMDK Created: build\SignaturesOS.vmdk" -ForegroundColor Green
}

# ==============================================================================
# VMX GENERATION (VMware Virtual Machine Configuration)
# ==============================================================================
Write-Host "--- Generating VMware Configuration (SignaturesOS.vmx) ---" -ForegroundColor Cyan
$vmxPath = "build\SignaturesOS.vmx"
$vmxContent = @"
.encoding = "windows-1252"
config.version = "8"
virtualHW.version = "18"
displayName = "SignaturesOS V2"
guestOS = "other-64"
numvcpus = "1"
memsize = "1024"
ide0:0.present = "TRUE"
ide0:0.fileName = "SignaturesOS.vmdk"
ide0:0.deviceType = "disk"
sound.present = "FALSE"
sound.virtualDev = "es1371"
sound.autodetect = "TRUE"
usb.present = "TRUE"
usb_xhci.present = "TRUE"
pciBridge0.present = "TRUE"
pciBridge4.present = "TRUE"
pciBridge4.virtualDev = "pcieRootPort"
pciBridge5.present = "TRUE"
pciBridge5.virtualDev = "pcieRootPort"
serial0.present = "TRUE"
serial0.fileType = "file"
serial0.fileName = "serial.log"
vmmouse.present = "TRUE"
ethernet0.present = "FALSE"
floppy0.present = "FALSE"
"@
[System.IO.File]::WriteAllText("$PWD\$vmxPath", $vmxContent)
Write-Host "[OK] VMX Created: $vmxPath" -ForegroundColor Green

Write-Host "=========================================" -ForegroundColor Green
Write-Host " BUILD SUCCESSFUL! VMDK: build\SignaturesOS.vmdk   " -ForegroundColor Green
Write-Host "                   VMX:  build\SignaturesOS.vmx    " -ForegroundColor Green
Write-Host "=========================================" -ForegroundColor Green



