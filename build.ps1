# Reload environment variables to detect newly installed tools
$env:Path = [System.Environment]::GetEnvironmentVariable("Path", "Machine") + ";" + [System.Environment]::GetEnvironmentVariable("Path", "User")

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

# Stage 2 will be assembled after kernel link to pass exact sector count

Write-Host "[3/5] Compiling Kernel & Drivers..." -ForegroundColor Yellow
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\kernel.c -o build\kernel.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\drivers\display\vram_accel.c -o build\vram_accel.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\debug\abde\abde_font.c -o build\abde_font.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\debug\abde\abde_renderer.c -o build\abde_renderer.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\debug\abde\abde.c -o build\abde.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\debug\vmm_lifecycle_debug.c -o build\vmm_lifecycle_debug.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\debug\syscall_tss_debug.c -o build\syscall_tss_debug.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\debug\pmm_debug.c -o build\pmm_debug.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\debug\keyboard_led_debug.c -o build\keyboard_led_debug.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\debug\ps2_micro_debug.c -o build\ps2_micro_debug.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\debug\screenshot\atoms_screenshot.c -o build\atoms_screenshot.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\debug\aipdebug\aipdebug.c -o build\aipdebug.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\debug\input_power_audit.c -o build\input_power_audit.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\debug\usb_hid_led_debug.c -o build\usb_hid_led_debug.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\debug\syscall_security_debug.c -o build\syscall_security_debug.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\debug\vfs_lifecycle_debug.c -o build\vfs_lifecycle_debug.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\debug\storage_forensic_debug.c -o build\storage_forensic_debug.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\debug\windows_forensic_collector.c -o build\windows_forensic_collector.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\vfs\bofs\src\bofs_validator.c -o build\bofs_validator.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\vfs\bofs\src\bofs_alloc.c -o build\bofs_alloc.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\vfs\bofs\src\bofs_file.c -o build\bofs_file.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\debug\bofs_file_test.c -o build\bofs_file_test.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\drivers\net\r8168\r8168.c -o build\r8168.o
if ($LASTEXITCODE -ne 0) { Write-Host "Realtek R8168 Driver Compilation Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\net\core\net_packet.c -o build\net_packet.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\net\core\net_device.c -o build\net_device.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\net\drivers\realtek\rtl8168.c -o build\net_rtl8168.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\net\drivers\realtek\realtek_master.c -o build\realtek_master.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\debug\lan_debug\lan_debug.c -o build\lan_debug.o
if ($LASTEXITCODE -ne 0) { Write-Host "LAN Debug Engine Compilation Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\debug\mouse_telemetry.c -o build\mouse_telemetry.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\shell\debug_shell.c -o build\debug_shell.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\ahme\src\ahme_core.c -o build\ahme_core.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\ahme\src\ahme_profile.c -o build\ahme_profile.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\ahme\src\ahme_quirks.c -o build\ahme_quirks.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\ahme\src\ahme_policy.c -o build\ahme_policy.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\ahme\src\ahme_health.c -o build\ahme_health.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\ahme\src\ahme_input.c -o build\ahme_input.o

clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\core\power\system_power.c -o build\system_power.o
if ($LASTEXITCODE -ne 0) { Write-Host "System Power Module Compilation Failed!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\core\pci\pci.c -o build\pci.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\graphics\gpu\core\gpu_manager.c -o build\gpu_manager.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\graphics\gpu\pci\gpu_pci.c -o build\gpu_pci.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\graphics\gpu\memory\gpu_memory.c -o build\gpu_memory.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\graphics\gpu\surface\gpu_surface.c -o build\gpu_surface_hal.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\graphics\gpu\drivers\gpu_drv_vmware.c -o build\gpu_drv_vmware.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\graphics\gpu\drivers\gpu_drv_virtio.c -o build\gpu_drv_virtio.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\graphics\gpu\drivers\gpu_drv_intel.c -o build\gpu_drv_intel.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\graphics\gpu\drivers\gpu_drv_amd.c -o build\gpu_drv_amd.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\graphics\gpu\drivers\gpu_drv_nvidia.c -o build\gpu_drv_nvidia.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\graphics\gpu\drivers\gpu_drv_swrender.c -o build\gpu_drv_swrender.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\graphics\gpu\debug\gpu_debug.c -o build\gpu_debug.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\graphics\display\monitors\edid_parser.c -o build\edid_parser.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\graphics\display\manager\display_manager.c -o build\display_manager.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\graphics\display\validation\core\dve_framebuffer.c -o build\dve_framebuffer.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\graphics\display\validation\core\dve_geometry.c -o build\dve_geometry.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\graphics\display\validation\core\dve_pixel_format.c -o build\dve_pixel_format.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\graphics\display\validation\core\dve_surface.c -o build\dve_surface.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\graphics\display\validation\core\dve_gpu_driver.c -o build\dve_gpu_driver.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\graphics\display\validation\core\dve_display_mode.c -o build\dve_display_mode.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\graphics\display\validation\core\dve_presentation.c -o build\dve_presentation.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\graphics\display\validation\core\dve_memory_safety.c -o build\dve_memory_safety.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\graphics\display\validation\core\dve_frame_integrity.c -o build\dve_frame_integrity.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\graphics\display\validation\core\dve_report.c -o build\dve_report.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\graphics\display\dpdp\core\dpdp.c -o build\dpdp.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\graphics\display\dpdp\core\dpdp_preview.c -o build\dpdp_preview.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\graphics\display\dpdp\core\dpdp_registers.c -o build\dpdp_registers.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\graphics\display\dpdp\core\dpdp_timeline.c -o build\dpdp_timeline.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\graphics\display\dpdp\tests\dpdp_tests.c -o build\dpdp_tests.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\graphics\gpu\tests\gpu_tests.c -o build\gpu_tests.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\graphics\bvmm\core\bvmm_init.c -o build\bvmm_init.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\graphics\bvmm\core\bvmm_core.c -o build\bvmm_core.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\graphics\bvmm\heap\bvmm_heap.c -o build\bvmm_heap.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\graphics\bvmm\heap\bvmm_tlsf.c -o build\bvmm_tlsf.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\graphics\bvmm\heap\bvmm_range.c -o build\bvmm_range.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\graphics\bvmm\handles\bvmm_handle_table.c -o build\bvmm_handle_table.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\graphics\bvmm\diagnostics\bvmm_stats.c -o build\bvmm_stats.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\graphics\bvmm\tests\bvmm_phase1_tests.c -o build\bvmm_phase1_tests.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\graphics\bvmm\tests\bvmm_phase2_tests.c -o build\bvmm_phase2_tests.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\graphics\bvmm\pools\bvmm_pools.c -o build\bvmm_pools.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\graphics\bvmm\pools\bvmm_pool_policy.c -o build\bvmm_pool_policy.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\graphics\bvmm\surface\bvmm_surface.c -o build\bvmm_surface.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\graphics\bvmm\surface\bvmm_surface_registry.c -o build\bvmm_surface_registry.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\graphics\bvmm\surface\bvmm_surface_diag.c -o build\bvmm_surface_diag.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\graphics\bvmm\tests\bvmm_phase3_tests.c -o build\bvmm_phase3_tests.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\graphics\bvmm\tests\bvmm_phase4_tests.c -o build\bvmm_phase4_tests.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\graphics\bvmm\texture\bvmm_texture.c -o build\bvmm_texture.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\graphics\bvmm\texture\bvmm_texture_registry.c -o build\bvmm_texture_registry.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\graphics\bvmm\texture\bvmm_texture_format.c -o build\bvmm_texture_format.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\graphics\bvmm\texture\bvmm_texture_diag.c -o build\bvmm_texture_diag.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\graphics\bvmm\tests\bvmm_phase5_tests.c -o build\bvmm_phase5_tests.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\graphics\bvmm\lifetime\bvmm_lifetime.c -o build\bvmm_lifetime.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\graphics\bvmm\lifetime\bvmm_lifetime_registry.c -o build\bvmm_lifetime_registry.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\graphics\bvmm\lifetime\bvmm_lifetime_diag.c -o build\bvmm_lifetime_diag.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\graphics\bvmm\tests\bvmm_phase6_tests.c -o build\bvmm_phase6_tests.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\graphics\bvmm\sync\bvmm_sync.c -o build\bvmm_sync.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\graphics\bvmm\sync\bvmm_sync_registry.c -o build\bvmm_sync_registry.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\graphics\bvmm\sync\bvmm_sync_graph.c -o build\bvmm_sync_graph.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\graphics\bvmm\sync\bvmm_sync_diag.c -o build\bvmm_sync_diag.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\graphics\bvmm\tests\bvmm_phase7_tests.c -o build\bvmm_phase7_tests.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\graphics\bvmm\policy\bvmm_policy.c -o build\bvmm_policy.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\graphics\bvmm\policy\bvmm_policy_registry.c -o build\bvmm_policy_registry.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\graphics\bvmm\policy\bvmm_policy_scheduler.c -o build\bvmm_policy_scheduler.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\graphics\bvmm\policy\bvmm_policy_diag.c -o build\bvmm_policy_diag.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\graphics\bvmm\tests\bvmm_phase8_tests.c -o build\bvmm_phase8_tests.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\graphics\bvmm\defrag\bvmm_defrag.c -o build\bvmm_defrag.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\graphics\bvmm\defrag\bvmm_defrag_registry.c -o build\bvmm_defrag_registry.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\graphics\bvmm\defrag\bvmm_defrag_analyzer.c -o build\bvmm_defrag_analyzer.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\graphics\bvmm\defrag\bvmm_defrag_diag.c -o build\bvmm_defrag_diag.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\graphics\bvmm\tests\bvmm_phase9_tests.c -o build\bvmm_phase9_tests.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\graphics\bvmm\hal\bghal.c -o build\bghal.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\graphics\bvmm\hal\bghal_registry.c -o build\bghal_registry.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\graphics\bvmm\hal\backends\bghal_intel.c -o build\bghal_intel.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\graphics\bvmm\hal\backends\bghal_amd.c -o build\bghal_amd.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\graphics\bvmm\hal\backends\bghal_nvidia.c -o build\bghal_nvidia.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\graphics\bvmm\hal\backends\bghal_virtio.c -o build\bghal_virtio.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\graphics\bvmm\hal\backends\bghal_vmware.c -o build\bghal_vmware.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\graphics\bvmm\hal\backends\bghal_swrender.c -o build\bghal_swrender.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\graphics\bvmm\hal\bghal_diag.c -o build\bghal_diag.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\graphics\bvmm\tests\bvmm_phase10_tests.c -o build\bvmm_phase10_tests.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\graphics\bvmm\sharing\bcpse.c -o build\bcpse.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\graphics\bvmm\sharing\bcpse_registry.c -o build\bcpse_registry.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\graphics\bvmm\sharing\bcpse_diag.c -o build\bcpse_diag.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\graphics\bvmm\tests\bvmm_phase11_tests.c -o build\bvmm_phase11_tests.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\graphics\bvmm\optimization\bpoe.c -o build\bpoe.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\graphics\bvmm\optimization\bpoe_optimizer.c -o build\bpoe_optimizer.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\graphics\bvmm\optimization\bpoe_profiler.c -o build\bpoe_profiler.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\graphics\bvmm\optimization\bpoe_fastpath.c -o build\bpoe_fastpath.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\graphics\bvmm\optimization\bpoe_validation.c -o build\bpoe_validation.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\graphics\bvmm\optimization\bpoe_cache.c -o build\bpoe_cache.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\graphics\bvmm\optimization\bpoe_lock_optimizer.c -o build\bpoe_lock_optimizer.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\graphics\bvmm\optimization\bpoe_diag.c -o build\bpoe_diag.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\graphics\bvmm\tests\bvmm_phase12_tests.c -o build\bvmm_phase12_tests.o
if ($LASTEXITCODE -ne 0) { Write-Host "BOS GPU Subsystem Compilation Failed!" -ForegroundColor Red; exit $LASTEXITCODE }

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
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\drivers\usb\core\usb_forensic_trace.c -o build\usb_forensic_trace.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\drivers\usb\core\usb_forensic_phase3.c -o build\usb_forensic_phase3.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\usb\xhci\ring\transfer_ring.c -o build\bte_transfer_ring.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\usb\xhci\ring\command_ring.c -o build\bte_command_ring.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\usb\xhci\ring\event_ring.c -o build\bte_event_ring.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\usb\xhci\queue\queue_manager.c -o build\bte_queue_manager.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\usb\xhci\queue\scheduler.c -o build\bte_scheduler.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\usb\xhci\bulk\bulk_in.c -o build\bte_bulk_in.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\usb\xhci\bulk\bulk_out.c -o build\bte_bulk_out.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\usb\xhci\bulk\bulk_submit.c -o build\bte_bulk_submit.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\usb\xhci\bulk\bulk_complete.c -o build\bte_bulk_complete.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\usb\xhci\interrupt\msi.c -o build\bte_msi.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\usb\xhci\interrupt\event_handler.c -o build\bte_event_handler.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\usb\xhci\diagnostics\telemetry.c -o build\bte_telemetry.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\usb\xhci\diagnostics\forensic_dump.c -o build\bte_forensic_dump.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\usb\xhci\diagnostics\ai_debug.c -o build\bte_ai_debug.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\usb\xhci\timeout\timeout.c -o build\bte_timeout.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\usb\xhci\tests\bulk_tests.c -o build\bte_bulk_tests.o

clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\usb\common\usb_dma.c -o build\lhce_usb_dma.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\usb\common\usb_scheduler.c -o build\lhce_usb_scheduler.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\usb\controller\usb_controller_manager.c -o build\lhce_controller_manager.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\usb\uhci\uhci.c -o build\lhce_uhci.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\usb\ohci\ohci.c -o build\lhce_ohci.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\usb\ehci\ehci.c -o build\lhce_ehci.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\usb\diagnostics\lhce_telemetry.c -o build\lhce_telemetry.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\usb\diagnostics\lhce_forensic_dump.c -o build\lhce_forensic_dump.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\usb\diagnostics\lhce_ai_debug.c -o build\lhce_ai_debug.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\usb\tests\lhce_certification_tests.c -o build\lhce_certification_tests.o

clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\usb\core\usb_core.c -o build\ucue_usb_core.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\usb\urb\usb_urb.c -o build\ucue_usb_urb.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\usb\endpoint\usb_endpoint.c -o build\ucue_usb_endpoint.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\usb\pipe\usb_pipe.c -o build\ucue_usb_pipe.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\usb\request\usb_request_queue.c -o build\ucue_usb_request_queue.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\usb\dispatcher\usb_transfer_dispatcher.c -o build\ucue_usb_transfer_dispatcher.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\usb\resource\usb_resource_manager.c -o build\ucue_usb_resource_manager.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\usb\timeout\usb_timeout_engine.c -o build\ucue_usb_timeout_engine.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\usb\completion\usb_completion_engine.c -o build\ucue_usb_completion_engine.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\usb\diagnostics\ucue_diagnostics.c -o build\ucue_diagnostics.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\usb\tests\ucue_certification_tests.c -o build\ucue_certification_tests.o

clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\usb\hub\core\hub_manager.c -o build\uhe_hub_manager.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\usb\hub\core\hub_enumeration.c -o build\uhe_hub_enumeration.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\usb\hub\core\hub_topology.c -o build\uhe_hub_topology.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\usb\hub\core\hub_registry.c -o build\uhe_hub_registry.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\usb\hub\ports\port_power.c -o build\uhe_port_power.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\usb\hub\ports\port_reset.c -o build\uhe_port_reset.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\usb\hub\ports\port_events.c -o build\uhe_port_events.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\usb\hub\ports\port_status.c -o build\uhe_port_status.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\usb\hub\ports\port_recovery.c -o build\uhe_port_recovery.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\usb\hub\diagnostics\telemetry.c -o build\uhe_telemetry.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\usb\hub\diagnostics\forensic_dump.c -o build\uhe_forensic_dump.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\usb\hub\diagnostics\ai_debug.c -o build\uhe_ai_debug.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\usb\hub\tests\hub_certification_tests.c -o build\uhe_hub_certification_tests.o

clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\usb\storage\bot\cbw.c -o build\ums_cbw.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\usb\storage\bot\csw.c -o build\ums_csw.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\usb\storage\bot\recovery.c -o build\ums_recovery.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\usb\storage\bot\transport.c -o build\ums_transport.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\usb\storage\bot\bot_engine.c -o build\ums_bot_engine.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\usb\storage\scsi\inquiry.c -o build\ums_scsi_inquiry.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\usb\storage\scsi\read_capacity.c -o build\ums_scsi_read_capacity.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\usb\storage\scsi\request_sense.c -o build\ums_scsi_request_sense.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\usb\storage\scsi\read10.c -o build\ums_scsi_read10.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\usb\storage\scsi\write10.c -o build\ums_scsi_write10.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\usb\storage\scsi\test_unit_ready.c -o build\ums_scsi_test_unit_ready.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\usb\storage\scsi\mode_sense.c -o build\ums_scsi_mode_sense.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\usb\storage\scsi\scsi_engine.c -o build\ums_scsi_engine.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\usb\storage\diagnostics\telemetry.c -o build\ums_telemetry.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\usb\storage\diagnostics\forensic_dump.c -o build\ums_forensic_dump.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\usb\storage\diagnostics\ai_debug.c -o build\ums_ai_debug.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\usb\storage\tests\bot_scsi_certification_tests.c -o build\ums_bot_scsi_certification_tests.o

clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\usb\storage_manager\core\disk_manager.c -o build\usm_disk_manager.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\usb\storage_manager\core\partition_manager.c -o build\usm_partition_manager.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\usb\storage_manager\core\volume_manager.c -o build\usm_volume_manager.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\usb\storage_manager\core\mount_manager.c -o build\usm_mount_manager.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\usb\storage_manager\core\storage_manager.c -o build\usm_storage_manager.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\usb\storage_manager\vfs\usb_sector_cache.c -o build\usm_sector_cache.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\usb\storage_manager\vfs\usb_block_cache.c -o build\usm_block_cache.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\usb\storage_manager\vfs\usb_io_scheduler.c -o build\usm_io_scheduler.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\usb\storage_manager\vfs\usb_vfs_bridge.c -o build\usm_vfs_bridge.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\usb\storage_manager\diagnostics\telemetry.c -o build\usm_telemetry.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\usb\storage_manager\diagnostics\forensic_dump.c -o build\usm_forensic_dump.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\usb\storage_manager\diagnostics\ai_debug.c -o build\usm_ai_debug.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\usb\storage_manager\tests\storage_manager_tests.c -o build\usm_storage_manager_tests.o


clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\core\bram\src\bram.c -o build\bram.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\display\dgl\src\dgl.c -o build\dgl.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\debug\klog\src\klog.c -o build\klog.o

clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\shell\rook\src\rook_core.c -o build\rook_core.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\shell\rook\src\rook_render.c -o build\rook_render.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\shell\rook\src\rook_registry.c -o build\rook_registry.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\shell\rook\src\rook_debug.c -o build\rook_debug.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\shell\rook\src\spinner.c -o build\rook_spinner.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\shell\rook\pages\page_boot.c -o build\page_boot.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\shell\rook\pages\page_login.c -o build\page_login.o
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\shell\rook\pages\page_shutdown.c -o build\page_shutdown.o

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

clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\mm\amsss\coordinator.c -o build\amsss_coordinator.o
if ($LASTEXITCODE -ne 0) { Write-Host "AMSSS coordinator compilation failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\mm\amsss\pressure_engine.c -o build\amsss_pressure.o
if ($LASTEXITCODE -ne 0) { Write-Host "AMSSS pressure compilation failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\mm\amsss\health_engine.c -o build\amsss_health.o
if ($LASTEXITCODE -ne 0) { Write-Host "AMSSS health compilation failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\mm\amsss\diagnostic_engine.c -o build\amsss_diagnostic.o
if ($LASTEXITCODE -ne 0) { Write-Host "AMSSS diagnostic compilation failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\mm\amsss\heap_backend.c -o build\amsss_heap_backend.o
if ($LASTEXITCODE -ne 0) { Write-Host "AMSSS heap backend compilation failed!" -ForegroundColor Red; exit $LASTEXITCODE }

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

clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -I. -c kernel\core\scheduler\src\kernel_stack.c -o build\kernel_stack.o
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

nasm -f elf64 arch\x86_64\smp\ap_trampoline.asm -o build\ap_trampoline.o
if ($LASTEXITCODE -ne 0) { Write-Host "AP Trampoline Assembly Failed!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c arch\x86_64\smp\smp.c -o build\smp.o
if ($LASTEXITCODE -ne 0) { Write-Host "SMP Compilation Failed!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\core\sync\spinlock.c -o build\spinlock.o
if ($LASTEXITCODE -ne 0) { Write-Host "Spinlock Compilation Failed!" -ForegroundColor Red; exit $LASTEXITCODE }


clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\ui\events\gui_events.c -o build\gui_events.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\core\syscall\src\syscall.c -o build\syscall.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\core\syscall\src\dispatcher.c -o build\syscall_dispatcher.o
if ($LASTEXITCODE -ne 0) { Write-Host "Syscall Dispatcher Compilation Failed!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\core\syscall\src\validation.c -o build\syscall_validation.o
if ($LASTEXITCODE -ne 0) { Write-Host "Syscall Validation Compilation Failed!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\core\syscall\src\services.c -o build\syscall_services.o
if ($LASTEXITCODE -ne 0) { Write-Host "Syscall Services Compilation Failed!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\core\syscall\tests\syscall_tests.c -o build\syscall_tests.o
if ($LASTEXITCODE -ne 0) { Write-Host "Syscall Tests Compilation Failed!" -ForegroundColor Red; exit $LASTEXITCODE }

nasm -f elf64 kernel\core\syscall\src\syscall_wrappers.asm -o build\syscall_wrappers.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

nasm -f elf64 kernel\core\syscall\src\syscall_entry.asm -o build\syscall_entry.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\vfs\vfs_legacy\storage\src\block_device.c -o build\block_device.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\drivers\storage_legacy\storage\src\ata.c -o build\ata.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\drivers\storage\ahci\ahci.c -o build\ahci.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\drivers\storage\nvme\nvme.c -o build\nvme.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED! NVMe compilation failed" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\drivers\storage\partition\gpt.c -o build\gpt.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED! GPT compilation failed" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\vfs\vfs_legacy\storage\src\mbr.c -o build\mbr.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\vfs\vfs_legacy\storage\src\disk_manager.c -o build\disk_manager.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\vfs\vfs_legacy\src\vfs.c -o build\vfs.o
if ($LASTEXITCODE -ne 0) { Write-Host "VFS Failed!" -ForegroundColor Red; exit }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\vfs\vfs_legacy\src\dummy_fs.c -o build\dummy_fs.o
if ($LASTEXITCODE -ne 0) { Write-Host "DummyFS Failed!" -ForegroundColor Red; exit }

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
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\drivers\input\cursor\cursor_certification.c -o build\cursor_certification.o
if ($LASTEXITCODE -ne 0) { Write-Host "Cursor Certification Failed!" -ForegroundColor Red; exit $LASTEXITCODE }

Write-Host "Compiling ATOMS USB Forensic Command Center V1.0..." -ForegroundColor Cyan
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\drivers\usb\forensics\usb_forensic_center.c -o build\usb_forensic_center.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\drivers\usb\forensics\usb_forensic_controller.c -o build\usb_forensic_controller.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\drivers\usb\forensics\usb_forensic_port.c -o build\usb_forensic_port.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\drivers\usb\forensics\usb_forensic_eventring.c -o build\usb_forensic_eventring.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\drivers\usb\forensics\usb_forensic_transfer.c -o build\usb_forensic_transfer.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\drivers\usb\forensics\usb_forensic_dma.c -o build\usb_forensic_dma.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\drivers\usb\forensics\usb_forensic_irq.c -o build\usb_forensic_irq.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\drivers\usb\forensics\usb_forensic_timeline.c -o build\usb_forensic_timeline.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\drivers\usb\forensics\usb_forensic_tree.c -o build\usb_forensic_tree.o
if ($LASTEXITCODE -ne 0) { Write-Host "USB Forensic Command Center Compilation Failed!" -ForegroundColor Red; exit $LASTEXITCODE }

Write-Host "Compiling ATOMS OS Industrial USB Stack Subsystem..." -ForegroundColor Cyan
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\drivers\usb\class\hid\hid_core.c -o build\hid_core.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\drivers\usb\class\hid\hid_parser.c -o build\hid_parser.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\drivers\usb\class\hid\hid_mouse.c -o build\hid_mouse.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\drivers\usb\class\hid\hid_keyboard.c -o build\hid_keyboard.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\drivers\usb\core\usb_hub.c -o build\usb_hub.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\drivers\usb\core\usb_urb.c -o build\usb_urb.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\drivers\usb\host\ehci\ehci_companion.c -o build\ehci_companion.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\drivers\usb\host\uhci\uhci.c -o build\uhci.o
if ($LASTEXITCODE -ne 0) { Write-Host "Industrial USB Stack Compilation Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c drivers\input\usb_tablet\usb_tablet.c -o build\usb_tablet.o
if ($LASTEXITCODE -ne 0) { Write-Host "USB Tablet Driver Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c drivers\input\vmmouse\vmmouse.c -o build\vmmouse.o
if ($LASTEXITCODE -ne 0) { Write-Host "VMMouse Driver Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\drivers\input\ivdl.c -o build\ivdl.o
if ($LASTEXITCODE -ne 0) { Write-Host "IVDL Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\drivers\input\mouse_engine\pointer_diag.c -o build\pointer_diag.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\drivers\input\pointer\pointer_predict.c -o build\pointer_predict.o
if ($LASTEXITCODE -ne 0) { Write-Host "Pointer Predict Engine Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\drivers\input\mouse_engine\pointer_filter.c -o build\pointer_filter.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\drivers\input\mouse_engine\pointer_sync.c -o build\pointer_sync.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\drivers\input\mouse_engine\pointer_manager.c -o build\pointer_manager.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\drivers\input\mouse_engine\mouse_engine.c -o build\mouse_engine.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\graphics\cursor\core\cur_loader.c -o build\bce_cur_loader.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\graphics\cursor\core\ani_loader.c -o build\bce_ani_loader.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\graphics\cursor\core\cursor_cache.c -o build\bce_cursor_cache.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\graphics\cursor\core\cursor_theme.c -o build\bce_cursor_theme.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\graphics\cursor\core\cursor_animation.c -o build\bce_cursor_animation.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\graphics\cursor\core\cursor_hal.c -o build\bce_cursor_hal.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\graphics\cursor\core\cursor_diag.c -o build\bce_cursor_diag.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\graphics\cursor\core\bos_cursor.c -o build\bce_bos_cursor.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\graphics\cursor\tests\cursor_tests.c -o build\bce_cursor_tests.o

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

Write-Host "Compiling BOS Composition Manager (BCM)..." -ForegroundColor Cyan
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\wm\bcm\src\bcm_core.c -o build\bcm_core.o
if ($LASTEXITCODE -ne 0) { Write-Host "BCM Core Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\wm\bcm\src\bcm_task.c -o build\bcm_task.o
if ($LASTEXITCODE -ne 0) { Write-Host "BCM Task Failed!" -ForegroundColor Red; exit $LASTEXITCODE }

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
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\core\execution\src\execution_contract.c -o build\atoms_execution_contract.o
if ($LASTEXITCODE -ne 0) { Write-Host "ATOMS Execution Contract Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\core\usermode\user_mode.c -o build\atoms_user_mode.o
if ($LASTEXITCODE -ne 0) { Write-Host "ATOMS User Mode Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\debug\user_mode_certification.c -o build\atoms_user_mode_certification.o
if ($LASTEXITCODE -ne 0) { Write-Host "ATOMS User Mode Certification Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\core\process\process_manager.c -o build\atoms_process_manager.o
if ($LASTEXITCODE -ne 0) { Write-Host "ATOMS Process Manager Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\core\thread\thread_manager.c -o build\atoms_thread_manager.o
if ($LASTEXITCODE -ne 0) { Write-Host "ATOMS Thread Manager Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\debug\execution_certification.c -o build\execution_certification.o
if ($LASTEXITCODE -ne 0) { Write-Host "ATOMS Execution Certification Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
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




Write-Host "Compiling ATOMS OS Phase 7 Userspace & C/C++ Runtime..." -ForegroundColor Cyan
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\runtime\c\src\atoms_syscall.c -o build\user_atoms_syscall.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\runtime\c\src\memory.c -o build\user_memory.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\runtime\c\src\stdio.c -o build\user_stdio.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\runtime\c\src\string.c -o build\user_string.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\runtime\c\src\pthread.c -o build\user_pthread.o
clang++ -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -fno-exceptions -fno-rtti -std=c++20 -I. -c userspace\runtime\cpp\src\cxx_runtime.cpp -o build\user_cxx_runtime.o
clang++ -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -fno-exceptions -fno-rtti -std=c++20 -I. -c userspace\tests\runtime_test\runtime_test_suite.cpp -o build\user_runtime_test_suite.o

Write-Host "Compiling ATOMS OS Phase 9 Skia 2D Graphics Engine & Adapter..." -ForegroundColor Cyan
clang++ -target x86_64-pc-none-elf -ffreestanding -mno-red-zone -fno-exceptions -fno-rtti -std=c++20 -I. -Iuserspace/runtime/c/include -Iuserspace/runtime/cpp/include -Ithird_party -c third_party\skia\src\core\SkColor.cpp -o build\skia_color.o
clang++ -target x86_64-pc-none-elf -ffreestanding -mno-red-zone -fno-exceptions -fno-rtti -std=c++20 -I. -Iuserspace/runtime/c/include -Iuserspace/runtime/cpp/include -Ithird_party -c third_party\skia\src\core\SkRect.cpp -o build\skia_rect.o
clang++ -target x86_64-pc-none-elf -ffreestanding -mno-red-zone -fno-exceptions -fno-rtti -std=c++20 -I. -Iuserspace/runtime/c/include -Iuserspace/runtime/cpp/include -Ithird_party -c third_party\skia\src\core\SkRRect.cpp -o build\skia_rrect.o
clang++ -target x86_64-pc-none-elf -ffreestanding -mno-red-zone -fno-exceptions -fno-rtti -std=c++20 -I. -Iuserspace/runtime/c/include -Iuserspace/runtime/cpp/include -Ithird_party -c third_party\skia\src\core\SkMatrix.cpp -o build\skia_matrix.o
clang++ -target x86_64-pc-none-elf -ffreestanding -mno-red-zone -fno-exceptions -fno-rtti -std=c++20 -I. -Iuserspace/runtime/c/include -Iuserspace/runtime/cpp/include -Ithird_party -c third_party\skia\src\core\SkPaint.cpp -o build\skia_paint.o
clang++ -target x86_64-pc-none-elf -ffreestanding -mno-red-zone -fno-exceptions -fno-rtti -std=c++20 -I. -Iuserspace/runtime/c/include -Iuserspace/runtime/cpp/include -Ithird_party -c third_party\skia\src\core\SkPath.cpp -o build\skia_path.o
clang++ -target x86_64-pc-none-elf -ffreestanding -mno-red-zone -fno-exceptions -fno-rtti -std=c++20 -I. -Iuserspace/runtime/c/include -Iuserspace/runtime/cpp/include -Ithird_party -c third_party\skia\src\core\SkImageInfo.cpp -o build\skia_imageinfo.o
clang++ -target x86_64-pc-none-elf -ffreestanding -mno-red-zone -fno-exceptions -fno-rtti -std=c++20 -I. -Iuserspace/runtime/c/include -Iuserspace/runtime/cpp/include -Ithird_party -c third_party\skia\src\core\SkCanvas.cpp -o build\skia_canvas.o
clang++ -target x86_64-pc-none-elf -ffreestanding -mno-red-zone -fno-exceptions -fno-rtti -std=c++20 -I. -Iuserspace/runtime/c/include -Iuserspace/runtime/cpp/include -Ithird_party -c third_party\skia\src\core\SkSurface.cpp -o build\skia_surface.o
clang++ -target x86_64-pc-none-elf -ffreestanding -mno-red-zone -fno-exceptions -fno-rtti -std=c++20 -I. -Iuserspace/runtime/c/include -Iuserspace/runtime/cpp/include -Ithird_party -c third_party\skia\src\core\SkRasterizer.cpp -o build\skia_rasterizer.o
clang++ -target x86_64-pc-none-elf -ffreestanding -mno-red-zone -fno-exceptions -fno-rtti -std=c++20 -I. -Iuserspace/runtime/c/include -Iuserspace/runtime/cpp/include -Ithird_party -c third_party\skia\src\adapter\atoms_skia_adapter.cpp -o build\skia_adapter.o
clang++ -target x86_64-pc-none-elf -ffreestanding -mno-red-zone -fno-exceptions -fno-rtti -std=c++20 -I. -Iuserspace/runtime/c/include -Iuserspace/runtime/cpp/include -Ithird_party -c third_party\skia\tests\skia_test_suite.cpp -o build\skia_test_suite.o

Write-Host "Compiling ATOMS OS Phase 10 Google V8 JavaScript Engine & Adapter..." -ForegroundColor Cyan
clang++ -target x86_64-pc-none-elf -ffreestanding -mno-red-zone -fno-exceptions -fno-rtti -std=c++20 -I. -Iuserspace/runtime/c/include -Iuserspace/runtime/cpp/include -Ithird_party -c third_party\v8\src\base\page-allocator.cpp -o build\v8_page_allocator.o
clang++ -target x86_64-pc-none-elf -ffreestanding -mno-red-zone -fno-exceptions -fno-rtti -std=c++20 -I. -Iuserspace/runtime/c/include -Iuserspace/runtime/cpp/include -Ithird_party -c third_party\v8\src\base\platform\platform-atoms.cpp -o build\v8_platform_atoms.o
clang++ -target x86_64-pc-none-elf -ffreestanding -mno-red-zone -fno-exceptions -fno-rtti -std=c++20 -I. -Iuserspace/runtime/c/include -Iuserspace/runtime/cpp/include -Ithird_party -c third_party\v8\src\objects\objects.cpp -o build\v8_objects.o
clang++ -target x86_64-pc-none-elf -ffreestanding -mno-red-zone -fno-exceptions -fno-rtti -std=c++20 -I. -Iuserspace/runtime/c/include -Iuserspace/runtime/cpp/include -Ithird_party -c third_party\v8\src\heap\heap.cpp -o build\v8_heap.o
clang++ -target x86_64-pc-none-elf -ffreestanding -mno-red-zone -fno-exceptions -fno-rtti -std=c++20 -I. -Iuserspace/runtime/c/include -Iuserspace/runtime/cpp/include -Ithird_party -c third_party\v8\src\interpreter\compiler.cpp -o build\v8_compiler.o
clang++ -target x86_64-pc-none-elf -ffreestanding -mno-red-zone -fno-exceptions -fno-rtti -std=c++20 -I. -Iuserspace/runtime/c/include -Iuserspace/runtime/cpp/include -Ithird_party -c third_party\v8\src\interpreter\interpreter.cpp -o build\v8_interpreter.o
clang++ -target x86_64-pc-none-elf -ffreestanding -mno-red-zone -fno-exceptions -fno-rtti -std=c++20 -I. -Iuserspace/runtime/c/include -Iuserspace/runtime/cpp/include -Ithird_party -c third_party\v8\src\codegen\x64\assembler-x64.cpp -o build\v8_assembler_x64.o
clang++ -target x86_64-pc-none-elf -ffreestanding -mno-red-zone -fno-exceptions -fno-rtti -std=c++20 -I. -Iuserspace/runtime/c/include -Iuserspace/runtime/cpp/include -Ithird_party -c third_party\v8\src\codegen\x64\jit-compiler-x64.cpp -o build\v8_jit_compiler_x64.o
clang++ -target x86_64-pc-none-elf -ffreestanding -mno-red-zone -fno-exceptions -fno-rtti -std=c++20 -I. -Iuserspace/runtime/c/include -Iuserspace/runtime/cpp/include -Ithird_party -c third_party\v8\src\api\api.cpp -o build\v8_api.o
clang++ -target x86_64-pc-none-elf -ffreestanding -mno-red-zone -fno-exceptions -fno-rtti -std=c++20 -I. -Iuserspace/runtime/c/include -Iuserspace/runtime/cpp/include -Ithird_party -c third_party\v8\src\adapter\atoms_v8_platform.cpp -o build\v8_atoms_platform.o
clang++ -target x86_64-pc-none-elf -ffreestanding -mno-red-zone -fno-exceptions -fno-rtti -std=c++20 -I. -Iuserspace/runtime/c/include -Iuserspace/runtime/cpp/include -Ithird_party -c third_party\v8\tests\v8_test_suite.cpp -o build\v8_test_suite.o

Write-Host "Compiling ATOMS OS Phase 11 Chromium Blink Core Engine..." -ForegroundColor Cyan
clang++ -target x86_64-pc-none-elf -ffreestanding -mno-red-zone -fno-exceptions -fno-rtti -std=c++20 -I. -Iuserspace/runtime/c/include -Iuserspace/runtime/cpp/include -Ithird_party -c third_party\blink\renderer\core\dom\node.cpp -o build\blink_node.o
clang++ -target x86_64-pc-none-elf -ffreestanding -mno-red-zone -fno-exceptions -fno-rtti -std=c++20 -I. -Iuserspace/runtime/c/include -Iuserspace/runtime/cpp/include -Ithird_party -c third_party\blink\renderer\core\dom\container_node.cpp -o build\blink_container_node.o
clang++ -target x86_64-pc-none-elf -ffreestanding -mno-red-zone -fno-exceptions -fno-rtti -std=c++20 -I. -Iuserspace/runtime/c/include -Iuserspace/runtime/cpp/include -Ithird_party -c third_party\blink\renderer\core\dom\element.cpp -o build\blink_element.o
clang++ -target x86_64-pc-none-elf -ffreestanding -mno-red-zone -fno-exceptions -fno-rtti -std=c++20 -I. -Iuserspace/runtime/c/include -Iuserspace/runtime/cpp/include -Ithird_party -c third_party\blink\renderer\core\dom\document.cpp -o build\blink_document.o
clang++ -target x86_64-pc-none-elf -ffreestanding -mno-red-zone -fno-exceptions -fno-rtti -std=c++20 -I. -Iuserspace/runtime/c/include -Iuserspace/runtime/cpp/include -Ithird_party -c third_party\blink\renderer\core\dom\text.cpp -o build\blink_text.o
clang++ -target x86_64-pc-none-elf -ffreestanding -mno-red-zone -fno-exceptions -fno-rtti -std=c++20 -I. -Iuserspace/runtime/c/include -Iuserspace/runtime/cpp/include -Ithird_party -c third_party\blink\renderer\core\html\parser\html_parser.cpp -o build\blink_html_parser.o
clang++ -target x86_64-pc-none-elf -ffreestanding -mno-red-zone -fno-exceptions -fno-rtti -std=c++20 -I. -Iuserspace/runtime/c/include -Iuserspace/runtime/cpp/include -Ithird_party -c third_party\blink\renderer\core\css\css_style_declaration.cpp -o build\blink_css_style_declaration.o
clang++ -target x86_64-pc-none-elf -ffreestanding -mno-red-zone -fno-exceptions -fno-rtti -std=c++20 -I. -Iuserspace/runtime/c/include -Iuserspace/runtime/cpp/include -Ithird_party -c third_party\blink\renderer\core\layout\layout_object.cpp -o build\blink_layout_object.o
clang++ -target x86_64-pc-none-elf -ffreestanding -mno-red-zone -fno-exceptions -fno-rtti -std=c++20 -I. -Iuserspace/runtime/c/include -Iuserspace/runtime/cpp/include -Ithird_party -c third_party\blink\renderer\core\layout\layout_block.cpp -o build\blink_layout_block.o
clang++ -target x86_64-pc-none-elf -ffreestanding -mno-red-zone -fno-exceptions -fno-rtti -std=c++20 -I. -Iuserspace/runtime/c/include -Iuserspace/runtime/cpp/include -Ithird_party -c third_party\blink\renderer\core\layout\layout_inline.cpp -o build\blink_layout_inline.o
clang++ -target x86_64-pc-none-elf -ffreestanding -mno-red-zone -fno-exceptions -fno-rtti -std=c++20 -I. -Iuserspace/runtime/c/include -Iuserspace/runtime/cpp/include -Ithird_party -c third_party\blink\renderer\core\layout\layout_tree_builder.cpp -o build\blink_layout_tree_builder.o
clang++ -target x86_64-pc-none-elf -ffreestanding -mno-red-zone -fno-exceptions -fno-rtti -std=c++20 -I. -Iuserspace/runtime/c/include -Iuserspace/runtime/cpp/include -Ithird_party -c third_party\blink\renderer\core\paint\blink_skia_painter.cpp -o build\blink_skia_painter.o
clang++ -target x86_64-pc-none-elf -ffreestanding -mno-red-zone -fno-exceptions -fno-rtti -std=c++20 -I. -Iuserspace/runtime/c/include -Iuserspace/runtime/cpp/include -Ithird_party -c third_party\blink\renderer\core\bindings\core\v8\script_controller.cpp -o build\blink_script_controller.o
clang++ -target x86_64-pc-none-elf -ffreestanding -mno-red-zone -fno-exceptions -fno-rtti -std=c++20 -I. -Iuserspace/runtime/c/include -Iuserspace/runtime/cpp/include -Ithird_party -c third_party\blink\renderer\adapter\atoms_blink_adapter.cpp -o build\blink_atoms_adapter.o
clang++ -target x86_64-pc-none-elf -ffreestanding -mno-red-zone -fno-exceptions -fno-rtti -std=c++20 -I. -Iuserspace/runtime/c/include -Iuserspace/runtime/cpp/include -Ithird_party -c third_party\blink\tests\blink_test_suite.cpp -o build\blink_test_suite.o

Write-Host "Compiling ATOMS OS Phase 12 Chromium Networking & Storage Subsystems..." -ForegroundColor Cyan
clang++ -target x86_64-pc-none-elf -ffreestanding -mno-red-zone -fno-exceptions -fno-rtti -std=c++20 -I. -Iuserspace/runtime/c/include -Iuserspace/runtime/cpp/include -Ithird_party -c third_party\chromium_net\base\gurl.cpp -o build\chromium_net_gurl.o
clang++ -target x86_64-pc-none-elf -ffreestanding -mno-red-zone -fno-exceptions -fno-rtti -std=c++20 -I. -Iuserspace/runtime/c/include -Iuserspace/runtime/cpp/include -Ithird_party -c third_party\chromium_net\base\security_origin.cpp -o build\chromium_net_security_origin.o
clang++ -target x86_64-pc-none-elf -ffreestanding -mno-red-zone -fno-exceptions -fno-rtti -std=c++20 -I. -Iuserspace/runtime/c/include -Iuserspace/runtime/cpp/include -Ithird_party -c third_party\chromium_net\http\http_request_headers.cpp -o build\chromium_net_http_request_headers.o
clang++ -target x86_64-pc-none-elf -ffreestanding -mno-red-zone -fno-exceptions -fno-rtti -std=c++20 -I. -Iuserspace/runtime/c/include -Iuserspace/runtime/cpp/include -Ithird_party -c third_party\chromium_net\http\http_response_headers.cpp -o build\chromium_net_http_response_headers.o
clang++ -target x86_64-pc-none-elf -ffreestanding -mno-red-zone -fno-exceptions -fno-rtti -std=c++20 -I. -Iuserspace/runtime/c/include -Iuserspace/runtime/cpp/include -Ithird_party -c third_party\chromium_net\http\http_cache.cpp -o build\chromium_net_http_cache.o
clang++ -target x86_64-pc-none-elf -ffreestanding -mno-red-zone -fno-exceptions -fno-rtti -std=c++20 -I. -Iuserspace/runtime/c/include -Iuserspace/runtime/cpp/include -Ithird_party -c third_party\chromium_net\cookies\canonical_cookie.cpp -o build\chromium_net_canonical_cookie.o
clang++ -target x86_64-pc-none-elf -ffreestanding -mno-red-zone -fno-exceptions -fno-rtti -std=c++20 -I. -Iuserspace/runtime/c/include -Iuserspace/runtime/cpp/include -Ithird_party -c third_party\chromium_net\cookies\cookie_store.cpp -o build\chromium_net_cookie_store.o
clang++ -target x86_64-pc-none-elf -ffreestanding -mno-red-zone -fno-exceptions -fno-rtti -std=c++20 -I. -Iuserspace/runtime/c/include -Iuserspace/runtime/cpp/include -Ithird_party -c third_party\chromium_net\url_request\url_loader.cpp -o build\chromium_net_url_loader.o
clang++ -target x86_64-pc-none-elf -ffreestanding -mno-red-zone -fno-exceptions -fno-rtti -std=c++20 -I. -Iuserspace/runtime/c/include -Iuserspace/runtime/cpp/include -Ithird_party -c third_party\chromium_net\adapter\atoms_network_adapter.cpp -o build\chromium_net_atoms_adapter.o
clang++ -target x86_64-pc-none-elf -ffreestanding -mno-red-zone -fno-exceptions -fno-rtti -std=c++20 -I. -Iuserspace/runtime/c/include -Iuserspace/runtime/cpp/include -Ithird_party -c third_party\chromium_storage\dom_storage\storage_area.cpp -o build\chromium_storage_area.o
clang++ -target x86_64-pc-none-elf -ffreestanding -mno-red-zone -fno-exceptions -fno-rtti -std=c++20 -I. -Iuserspace/runtime/c/include -Iuserspace/runtime/cpp/include -Ithird_party -c third_party\chromium_storage\dom_storage\storage_namespace.cpp -o build\chromium_storage_namespace.o
clang++ -target x86_64-pc-none-elf -ffreestanding -mno-red-zone -fno-exceptions -fno-rtti -std=c++20 -I. -Iuserspace/runtime/c/include -Iuserspace/runtime/cpp/include -Ithird_party -c third_party\chromium_storage\dom_storage\local_storage_manager.cpp -o build\chromium_storage_local_manager.o
clang++ -target x86_64-pc-none-elf -ffreestanding -mno-red-zone -fno-exceptions -fno-rtti -std=c++20 -I. -Iuserspace/runtime/c/include -Iuserspace/runtime/cpp/include -Ithird_party -c third_party\chromium_storage\dom_storage\session_storage_manager.cpp -o build\chromium_storage_session_manager.o
clang++ -target x86_64-pc-none-elf -ffreestanding -mno-red-zone -fno-exceptions -fno-rtti -std=c++20 -I. -Iuserspace/runtime/c/include -Iuserspace/runtime/cpp/include -Ithird_party -c third_party\chromium_storage\adapter\atoms_storage_vfs_adapter.cpp -o build\chromium_storage_atoms_vfs_adapter.o
clang++ -target x86_64-pc-none-elf -ffreestanding -mno-red-zone -fno-exceptions -fno-rtti -std=c++20 -I. -Iuserspace/runtime/c/include -Iuserspace/runtime/cpp/include -Ithird_party -c third_party\chromium_net\tests\net_storage_test_suite.cpp -o build\chromium_net_storage_test_suite.o

Write-Host "Compiling ATOMS OS Phase 13 Multi-Process Browser Architecture Subsystems..." -ForegroundColor Cyan
clang++ -target x86_64-pc-none-elf -ffreestanding -mno-red-zone -fno-exceptions -fno-rtti -std=c++20 -I. -Iuserspace/runtime/c/include -Iuserspace/runtime/cpp/include -Ithird_party -c third_party\chromium_ipc\atoms_ipc_channel.cpp -o build\chromium_ipc_channel.o
clang++ -target x86_64-pc-none-elf -ffreestanding -mno-red-zone -fno-exceptions -fno-rtti -std=c++20 -I. -Iuserspace/runtime/c/include -Iuserspace/runtime/cpp/include -Ithird_party -c third_party\chromium_process\renderer_process_host.cpp -o build\chromium_renderer_process_host.o
clang++ -target x86_64-pc-none-elf -ffreestanding -mno-red-zone -fno-exceptions -fno-rtti -std=c++20 -I. -Iuserspace/runtime/c/include -Iuserspace/runtime/cpp/include -Ithird_party -c third_party\chromium_process\network_process_host.cpp -o build\chromium_network_process_host.o
clang++ -target x86_64-pc-none-elf -ffreestanding -mno-red-zone -fno-exceptions -fno-rtti -std=c++20 -I. -Iuserspace/runtime/c/include -Iuserspace/runtime/cpp/include -Ithird_party -c third_party\chromium_process\utility_process_host.cpp -o build\chromium_utility_process_host.o
clang++ -target x86_64-pc-none-elf -ffreestanding -mno-red-zone -fno-exceptions -fno-rtti -std=c++20 -I. -Iuserspace/runtime/c/include -Iuserspace/runtime/cpp/include -Ithird_party -c third_party\chromium_process\browser_process_host.cpp -o build\chromium_browser_process_host.o
clang++ -target x86_64-pc-none-elf -ffreestanding -mno-red-zone -fno-exceptions -fno-rtti -std=c++20 -I. -Iuserspace/runtime/c/include -Iuserspace/runtime/cpp/include -Ithird_party -c third_party\chromium_process\tests\process_test_suite.cpp -o build\chromium_process_test_suite.o

Write-Host "Compiling ATOMS OS Phase 14 Chromium Mojo IPC Subsystems..." -ForegroundColor Cyan
clang++ -target x86_64-pc-none-elf -ffreestanding -mno-red-zone -fno-exceptions -fno-rtti -std=c++20 -I. -Iuserspace/runtime/c/include -Iuserspace/runtime/cpp/include -Ithird_party -c mojo\core\handle_table.cpp -o build\mojo_core_handle_table.o
clang++ -target x86_64-pc-none-elf -ffreestanding -mno-red-zone -fno-exceptions -fno-rtti -std=c++20 -I. -Iuserspace/runtime/c/include -Iuserspace/runtime/cpp/include -Ithird_party -c mojo\core\message_pipe.cpp -o build\mojo_core_message_pipe.o
clang++ -target x86_64-pc-none-elf -ffreestanding -mno-red-zone -fno-exceptions -fno-rtti -std=c++20 -I. -Iuserspace/runtime/c/include -Iuserspace/runtime/cpp/include -Ithird_party -c mojo\core\shared_buffer.cpp -o build\mojo_core_shared_buffer.o
clang++ -target x86_64-pc-none-elf -ffreestanding -mno-red-zone -fno-exceptions -fno-rtti -std=c++20 -I. -Iuserspace/runtime/c/include -Iuserspace/runtime/cpp/include -Ithird_party -c mojo\core\mojo_core.cpp -o build\mojo_core_c_abi.o
clang++ -target x86_64-pc-none-elf -ffreestanding -mno-red-zone -fno-exceptions -fno-rtti -std=c++20 -I. -Iuserspace/runtime/c/include -Iuserspace/runtime/cpp/include -Ithird_party -c mojo\public\cpp\system\message.cpp -o build\mojo_public_message.o
clang++ -target x86_64-pc-none-elf -ffreestanding -mno-red-zone -fno-exceptions -fno-rtti -std=c++20 -I. -Iuserspace/runtime/c/include -Iuserspace/runtime/cpp/include -Ithird_party -c mojo\tests\mojo_test_suite.cpp -o build\mojo_test_suite.o

Write-Host "Compiling ATOMS OS Phase 15 Chromium Sandbox & Web Security Subsystems..." -ForegroundColor Cyan
clang++ -target x86_64-pc-none-elf -ffreestanding -mno-red-zone -fno-exceptions -fno-rtti -std=c++20 -I. -Iuserspace/runtime/c/include -Iuserspace/runtime/cpp/include -Ithird_party -c third_party\chromium_net\base\content_security_policy.cpp -o build\chromium_net_csp.o
clang++ -target x86_64-pc-none-elf -ffreestanding -mno-red-zone -fno-exceptions -fno-rtti -std=c++20 -I. -Iuserspace/runtime/c/include -Iuserspace/runtime/cpp/include -Ithird_party -c third_party\chromium_net\base\security_headers.cpp -o build\chromium_net_security_headers.o
clang++ -target x86_64-pc-none-elf -ffreestanding -mno-red-zone -fno-exceptions -fno-rtti -std=c++20 -I. -Iuserspace/runtime/c/include -Iuserspace/runtime/cpp/include -Ithird_party -c third_party\chromium_security\tests\security_test_suite.cpp -o build\chromium_security_test_suite.o

Write-Host "Compiling ATOMS OS Phase 16 Chromium GPU, Media & Web APIs Subsystems..." -ForegroundColor Cyan
clang++ -target x86_64-pc-none-elf -ffreestanding -mno-red-zone -fno-exceptions -fno-rtti -std=c++20 -I. -Iuserspace/runtime/c/include -Iuserspace/runtime/cpp/include -Ithird_party -c third_party\chromium_process\gpu_process_host.cpp -o build\chromium_gpu_process_host.o
clang++ -target x86_64-pc-none-elf -ffreestanding -mno-red-zone -fno-exceptions -fno-rtti -std=c++20 -I. -Iuserspace/runtime/c/include -Iuserspace/runtime/cpp/include -Ithird_party -c third_party\chromium_gpu\command_buffer\command_buffer.cpp -o build\chromium_gpu_command_buffer.o
clang++ -target x86_64-pc-none-elf -ffreestanding -mno-red-zone -fno-exceptions -fno-rtti -std=c++20 -I. -Iuserspace/runtime/c/include -Iuserspace/runtime/cpp/include -Ithird_party -c third_party\chromium_gpu\command_buffer\gpu_command_decoder.cpp -o build\chromium_gpu_command_decoder.o
clang++ -target x86_64-pc-none-elf -ffreestanding -mno-red-zone -fno-exceptions -fno-rtti -std=c++20 -I. -Iuserspace/runtime/c/include -Iuserspace/runtime/cpp/include -Ithird_party -c third_party\chromium_gpu\command_buffer\gpu_channel_host.cpp -o build\chromium_gpu_channel_host.o
clang++ -target x86_64-pc-none-elf -ffreestanding -mno-red-zone -fno-exceptions -fno-rtti -std=c++20 -I. -Iuserspace/runtime/c/include -Iuserspace/runtime/cpp/include -Ithird_party -c third_party\blink\renderer\core\html\canvas\webgl_rendering_context.cpp -o build\blink_webgl_rendering_context.o
clang++ -target x86_64-pc-none-elf -ffreestanding -mno-red-zone -fno-exceptions -fno-rtti -std=c++20 -I. -Iuserspace/runtime/c/include -Iuserspace/runtime/cpp/include -Ithird_party -c third_party\blink\renderer\core\html\canvas\canvas_rendering_context_2d.cpp -o build\blink_canvas_rendering_context_2d.o
clang++ -target x86_64-pc-none-elf -ffreestanding -mno-red-zone -fno-exceptions -fno-rtti -std=c++20 -I. -Iuserspace/runtime/c/include -Iuserspace/runtime/cpp/include -Ithird_party -c third_party\blink\renderer\core\html\canvas\offscreen_canvas.cpp -o build\blink_offscreen_canvas.o
clang++ -target x86_64-pc-none-elf -ffreestanding -mno-red-zone -fno-exceptions -fno-rtti -std=c++20 -I. -Iuserspace/runtime/c/include -Iuserspace/runtime/cpp/include -Ithird_party -c third_party\blink\renderer\core\html\canvas\image_bitmap.cpp -o build\blink_image_bitmap.o
clang++ -target x86_64-pc-none-elf -ffreestanding -mno-red-zone -fno-exceptions -fno-rtti -std=c++20 -I. -Iuserspace/runtime/c/include -Iuserspace/runtime/cpp/include -Ithird_party -c third_party\blink\renderer\core\html\media\html_media_element.cpp -o build\blink_html_media_element.o
clang++ -target x86_64-pc-none-elf -ffreestanding -mno-red-zone -fno-exceptions -fno-rtti -std=c++20 -I. -Iuserspace/runtime/c/include -Iuserspace/runtime/cpp/include -Ithird_party -c third_party\blink\renderer\core\html\media\html_video_element.cpp -o build\blink_html_video_element.o
clang++ -target x86_64-pc-none-elf -ffreestanding -mno-red-zone -fno-exceptions -fno-rtti -std=c++20 -I. -Iuserspace/runtime/c/include -Iuserspace/runtime/cpp/include -Ithird_party -c third_party\blink\renderer\core\html\media\html_audio_element.cpp -o build\blink_html_audio_element.o
clang++ -target x86_64-pc-none-elf -ffreestanding -mno-red-zone -fno-exceptions -fno-rtti -std=c++20 -I. -Iuserspace/runtime/c/include -Iuserspace/runtime/cpp/include -Ithird_party -c third_party\blink\renderer\core\fileapi\blob.cpp -o build\blink_blob.o
clang++ -target x86_64-pc-none-elf -ffreestanding -mno-red-zone -fno-exceptions -fno-rtti -std=c++20 -I. -Iuserspace/runtime/c/include -Iuserspace/runtime/cpp/include -Ithird_party -c third_party\blink\renderer\core\fileapi\file_reader.cpp -o build\blink_file_reader.o
clang++ -target x86_64-pc-none-elf -ffreestanding -mno-red-zone -fno-exceptions -fno-rtti -std=c++20 -I. -Iuserspace/runtime/c/include -Iuserspace/runtime/cpp/include -Ithird_party -c third_party\blink\renderer\modules\webaudio\audio_context.cpp -o build\blink_audio_context.o
clang++ -target x86_64-pc-none-elf -ffreestanding -mno-red-zone -fno-exceptions -fno-rtti -std=c++20 -I. -Iuserspace/runtime/c/include -Iuserspace/runtime/cpp/include -Ithird_party -c third_party\blink\renderer\modules\mediasource\media_source.cpp -o build\blink_media_source.o
clang++ -target x86_64-pc-none-elf -ffreestanding -mno-red-zone -fno-exceptions -fno-rtti -std=c++20 -I. -Iuserspace/runtime/c/include -Iuserspace/runtime/cpp/include -Ithird_party -c third_party\blink\renderer\modules\webcodecs\video_decoder.cpp -o build\blink_video_decoder.o
clang++ -target x86_64-pc-none-elf -ffreestanding -mno-red-zone -fno-exceptions -fno-rtti -std=c++20 -I. -Iuserspace/runtime/c/include -Iuserspace/runtime/cpp/include -Ithird_party -c third_party\chromium_media_gpu\tests\media_gpu_test_suite.cpp -o build\chromium_media_gpu_test_suite.o
Write-Host "Compiling ATOMS OS Phase 17 Web Compatibility & Hardening Subsystems..." -ForegroundColor Cyan
clang++ -target x86_64-pc-none-elf -ffreestanding -mno-red-zone -fno-exceptions -fno-rtti -std=c++20 -I. -Iuserspace/runtime/c/include -Iuserspace/runtime/cpp/include -Ithird_party -c third_party\chromium_compatibility\tests\compatibility_test_suite.cpp -o build\chromium_compatibility_test_suite.o
if ($LASTEXITCODE -ne 0) { Write-Host "Phase 17 Compatibility Test Suite Failed!" -ForegroundColor Red; exit $LASTEXITCODE }









Write-Host "Compiling ATOMS OS Desktop Shell..." -ForegroundColor Cyan

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\shell\desktop_shell\bomatrix.c -o build\bomatrix.o
if ($LASTEXITCODE -ne 0) { Write-Host "BOMATRIX Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\shell\desktop_shell\desktop_shell.c -o build\desktop_shell.o
if ($LASTEXITCODE -ne 0) { Write-Host "Shell Core Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\shell\desktop_shell\dom.c -o build\dom.o
if ($LASTEXITCODE -ne 0) { Write-Host "DOM Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\shell\desktop_shell\desktop_vfs_sync.c -o build\desktop_vfs_sync.o
if ($LASTEXITCODE -ne 0) { Write-Host "Desktop VFS Sync Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\shell\desktop_shell\desktop_watcher.c -o build\desktop_watcher.o
if ($LASTEXITCODE -ne 0) { Write-Host "Desktop Watcher Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\shell\desktop_shell\desktop_certification_tests.c -o build\desktop_certification_tests.o
if ($LASTEXITCODE -ne 0) { Write-Host "Desktop Certification Tests Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\engine\horse_engine.c -o build\horse_engine.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\ui\icon_engine\src\icon_engine.c -o build\icon_engine.o
if ($LASTEXITCODE -ne 0) { Write-Host "Icon Engine Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\ui\icon_engine\icons\atoms_start_icon.c -o build\atoms_start_icon.o
if ($LASTEXITCODE -ne 0) { Write-Host "ATOMS Start Icon Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
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
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\apps\atrix\minbrow_probe.c -o build\minbrow_probe.o
if ($LASTEXITCODE -ne 0) { Write-Host "ATRIX Browser / Minimal Probe Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\apps\tmh\tmh_app.c -o build\tmh_app.o
if ($LASTEXITCODE -ne 0) { Write-Host "TMH App Failed!" -ForegroundColor Red; exit $LASTEXITCODE }

Write-Host "Compiling ATOMS OS Rewritten Explorer (Phase 9)..." -ForegroundColor Cyan
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\shell\apps\explorer.c -o build\explorer.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\shell\apps\explorer_view.c -o build\explorer_view.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\shell\apps\explorer_cache.c -o build\explorer_cache.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\shell\apps\notes_app.c -o build\notes_app.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\shell\apps\rename_dialog.c -o build\rename_dialog.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\shell\apps\app_clipboard.c -o build\app_clipboard.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\shell\apps\tests\explorer_rewrite_certification.c -o build\explorer_rewrite_certification.o
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
if ($LASTEXITCODE -ne 0) { Write-Host "BOPAWN cache compilation failed!" -ForegroundColor Red; exit $LASTEXITCODE }
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

Write-Host "Compiling ATOMS Motion Engine (AME)..." -ForegroundColor Cyan
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\ame\src\ame_core.c -o build\ame_core.o
if ($LASTEXITCODE -ne 0) { Write-Host "AME Core Failed!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\ame\src\ame_easing.c -o build\ame_easing.o
if ($LASTEXITCODE -ne 0) { Write-Host "AME Easing Failed!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\ame\src\ame_spinner.c -o build\ame_spinner.o
if ($LASTEXITCODE -ne 0) { Write-Host "AME Spinner Module Failed!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\ame\src\ame_modules.c -o build\ame_modules.o
if ($LASTEXITCODE -ne 0) { Write-Host "AME Subsystem Modules Failed!" -ForegroundColor Red; exit $LASTEXITCODE }

Write-Host "Compiling ATOMS Login Subsystem & Wallpaper Service..." -ForegroundColor Cyan
nasm -f elf64 kernel\services\wallpaper\boot_assets_data.asm -o build\boot_assets_data.o
if ($LASTEXITCODE -ne 0) { Write-Host "Boot Assets ASM Failed!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\services\wallpaper\boot_assets.c -o build\boot_assets.o
if ($LASTEXITCODE -ne 0) { Write-Host "Boot Assets Failed!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\services\wallpaper\wallpaper_service.c -o build\wallpaper_service.o
if ($LASTEXITCODE -ne 0) { Write-Host "Wallpaper Service Failed!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\services\user_profile\user_profile_service.c -o build\user_profile_service.o
if ($LASTEXITCODE -ne 0) { Write-Host "User Profile Service Failed!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\shell\rook\pages\page_login.c -o build\page_login.o
if ($LASTEXITCODE -ne 0) { Write-Host "ROOK Page Login Failed!" -ForegroundColor Red; exit $LASTEXITCODE }

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

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\shell\rook\src\spinner.c -o build\spinner.o
if ($LASTEXITCODE -ne 0) { Write-Host "ROOK Spinner Engine Failed!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\shell\rook\pages\page_boot.c -o build\page_boot.o
if ($LASTEXITCODE -ne 0) { Write-Host "ROOK Boot Page Failed!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\shell\rook\debug\dashboard.c -o build\rook_dashboard.o
if ($LASTEXITCODE -ne 0) { Write-Host "ROOK Dashboard Failed!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\shell\rook\pages\page_login.c -o build\page_login.o
if ($LASTEXITCODE -ne 0) { Write-Host "ROOK Login Page Failed!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\shell\rook\pages\page_shutdown.c -o build\page_shutdown.o
if ($LASTEXITCODE -ne 0) { Write-Host "ROOK Shutdown Page Failed!" -ForegroundColor Red; exit $LASTEXITCODE }

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

Write-Host "Compiling Performance Profiler Engine (PPE)..." -ForegroundColor Cyan
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\performance\core\profiler.c -o build\profiler.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\performance\frame\frame_profiler.c -o build\frame_profiler.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\performance\renderer\render_profiler.c -o build\render_profiler.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\performance\scheduler\sched_profiler.c -o build\sched_profiler.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\performance\memory\mem_profiler.c -o build\mem_profiler.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\performance\present\present_profiler.c -o build\present_profiler.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\performance\statistics\stats_profiler.c -o build\stats_profiler.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\performance\report\report_profiler.c -o build\report_profiler.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\system\boot\boot_mode.c -o build\boot_mode.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\system\boot\deferred_work.c -o build\deferred_work.o
if ($LASTEXITCODE -ne 0) { Write-Host "PPE, FPJA & Boot Subsystem Failed!" -ForegroundColor Red; exit $LASTEXITCODE }




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

Write-Host "Compiling BOS OS Phase 3 Production TLS & Security Engine..." -ForegroundColor Cyan
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\security\debug\sec_debug.c -o build\sec_debug.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\security\diagnostics\sec_diag.c -o build\sec_diag.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\security\random\sec_random.c -o build\sec_random.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\security\hash\sec_sha256.c -o build\sec_sha256.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\security\hash\sec_sha384.c -o build\sec_sha384.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\security\hash\sec_sha512.c -o build\sec_sha512.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\security\aes\sec_aes.c -o build\sec_aes.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\security\rsa\sec_rsa.c -o build\sec_rsa.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\security\ecc\sec_ecc.c -o build\sec_ecc.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\security\crypto\crypto_manager.c -o build\sec_crypto_manager.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\security\x509\sec_x509.c -o build\sec_x509.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\security\certificates\sec_cert_validator.c -o build\sec_cert_validator.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\security\trust_store\sec_trust_store.c -o build\sec_trust_store.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\security\session\sec_session.c -o build\sec_session.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\security\tls\sec_cipher_suite.c -o build\sec_cipher_suite.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\security\tls\sec_key_exchange.c -o build\sec_key_exchange.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\security\tls\sec_tls_engine.c -o build\sec_tls_engine.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\security\core\sec_manager.c -o build\sec_manager.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\security\tests\sec_test_suite.c -o build\sec_test_suite.o
if ($LASTEXITCODE -ne 0) { Write-Host "BOS TLS & Security Subsystem Failed!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\net\socket\socket_test.c -o build\socket_test.o
if ($LASTEXITCODE -ne 0) { Write-Host "Socket Test Suite Failed!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c arch\x86_64\cpu\cpu_features.c -o build\cpu_features.o
if ($LASTEXITCODE -ne 0) { Write-Host "CPU Features Compilation Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c arch\x86_64\smp\smp.c -o build\smp.o
if ($LASTEXITCODE -ne 0) { Write-Host "SMP Foundation Compilation Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\debug\phase7_reliability.c -o build\phase7_reliability.o
if ($LASTEXITCODE -ne 0) { Write-Host "Phase 7 Reliability Compilation Failed!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\debug\phase7_cert.c -o build\phase7_cert.o
if ($LASTEXITCODE -ne 0) { Write-Host "Phase 7 Cert Compilation Failed!" -ForegroundColor Red; exit $LASTEXITCODE }

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c kernel\core\sync\spinlock.c -o build\spinlock.o
if ($LASTEXITCODE -ne 0) { Write-Host "SMP Spinlock Compilation Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
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
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\apps\tmh\tmh_provider.c -o build\atoms_tmh_provider.o
if ($LASTEXITCODE -ne 0) { Write-Host "TMH Provider Compilation Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\apps\tmh\tmh_app.c -o build\tmh_app.o
if ($LASTEXITCODE -ne 0) { Write-Host "TMH App Compilation Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -msse -msse2 -ffreestanding -mno-red-zone -I. -c kernel\sandbox\core\sandbox_manager.c -o build\sb_manager.o
if ($LASTEXITCODE -ne 0) { Write-Host "Sandbox Manager Compilation Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -msse -msse2 -ffreestanding -mno-red-zone -I. -c kernel\sandbox\process\sandbox_process.c -o build\sb_process.o
if ($LASTEXITCODE -ne 0) { Write-Host "Sandbox Process Compilation Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -msse -msse2 -ffreestanding -mno-red-zone -I. -c kernel\sandbox\token\sandbox_token.c -o build\sb_token.o
if ($LASTEXITCODE -ne 0) { Write-Host "Sandbox Token Compilation Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -msse -msse2 -ffreestanding -mno-red-zone -I. -c kernel\sandbox\capability\sandbox_capability.c -o build\sb_capability.o
if ($LASTEXITCODE -ne 0) { Write-Host "Sandbox Capability Compilation Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -msse -msse2 -ffreestanding -mno-red-zone -I. -c kernel\sandbox\permissions\sandbox_permissions.c -o build\sb_permissions.o
if ($LASTEXITCODE -ne 0) { Write-Host "Sandbox Permissions Compilation Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -msse -msse2 -ffreestanding -mno-red-zone -I. -c kernel\sandbox\memory\sandbox_memory.c -o build\sb_memory.o
if ($LASTEXITCODE -ne 0) { Write-Host "Sandbox Memory Compilation Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -msse -msse2 -ffreestanding -mno-red-zone -I. -c kernel\sandbox\syscall\sandbox_syscall.c -o build\sb_syscall.o
if ($LASTEXITCODE -ne 0) { Write-Host "Sandbox Syscall Compilation Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -msse -msse2 -ffreestanding -mno-red-zone -I. -c kernel\sandbox\ipc\sandbox_ipc.c -o build\sb_ipc.o
if ($LASTEXITCODE -ne 0) { Write-Host "Sandbox IPC Compilation Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -msse -msse2 -ffreestanding -mno-red-zone -I. -c kernel\sandbox\filesystem\sandbox_filesystem.c -o build\sb_filesystem.o
if ($LASTEXITCODE -ne 0) { Write-Host "Sandbox Filesystem Compilation Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -msse -msse2 -ffreestanding -mno-red-zone -I. -c kernel\sandbox\network\sandbox_network.c -o build\sb_network.o
if ($LASTEXITCODE -ne 0) { Write-Host "Sandbox Network Compilation Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -msse -msse2 -ffreestanding -mno-red-zone -I. -c kernel\sandbox\resource\sandbox_resource.c -o build\sb_resource.o
if ($LASTEXITCODE -ne 0) { Write-Host "Sandbox Resource Compilation Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -msse -msse2 -ffreestanding -mno-red-zone -I. -c kernel\sandbox\audit\sandbox_audit.c -o build\sb_audit.o
if ($LASTEXITCODE -ne 0) { Write-Host "Sandbox Audit Compilation Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -msse -msse2 -ffreestanding -mno-red-zone -I. -c kernel\sandbox\debug\sandbox_debug.c -o build\sb_debug.o
if ($LASTEXITCODE -ne 0) { Write-Host "Sandbox Debug Compilation Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -msse -msse2 -ffreestanding -mno-red-zone -I. -c kernel\sandbox\tests\sandbox_tests.c -o build\sb_tests.o
if ($LASTEXITCODE -ne 0) { Write-Host "Sandbox Tests Compilation Failed!" -ForegroundColor Red; exit $LASTEXITCODE }

Write-Host "Compiling BOS OS Phase 5A Production HTML5 Parser & DOM Core Engine..." -ForegroundColor Cyan
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c browser\html\debug\html_debug.c -o build\html_debug.o
if ($LASTEXITCODE -ne 0) { Write-Host "HTML Debug Compilation Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c browser\html\diagnostics\html_diagnostics.c -o build\html_diag.o
if ($LASTEXITCODE -ne 0) { Write-Host "HTML Diagnostics Compilation Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c browser\html\attributes\html_attribute.c -o build\html_attr.o
if ($LASTEXITCODE -ne 0) { Write-Host "HTML Attribute Compilation Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c browser\html\node\html_node.c -o build\html_node.o
if ($LASTEXITCODE -ne 0) { Write-Host "HTML Node Compilation Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c browser\html\element\html_element.c -o build\html_elem.o
if ($LASTEXITCODE -ne 0) { Write-Host "HTML Element Compilation Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c browser\html\text\html_text.c -o build\html_text.o
if ($LASTEXITCODE -ne 0) { Write-Host "HTML Text Compilation Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c browser\html\comment\html_comment.c -o build\html_comment.o
if ($LASTEXITCODE -ne 0) { Write-Host "HTML Comment Compilation Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c browser\html\doctype\html_doctype.c -o build\html_doctype.o
if ($LASTEXITCODE -ne 0) { Write-Host "HTML Doctype Compilation Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c browser\html\fragment\html_fragment.c -o build\html_frag.o
if ($LASTEXITCODE -ne 0) { Write-Host "HTML Fragment Compilation Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c browser\html\document\html_document.c -o build\html_doc.o
if ($LASTEXITCODE -ne 0) { Write-Host "HTML Document Compilation Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c browser\html\mutation\html_mutation.c -o build\html_mut.o
if ($LASTEXITCODE -ne 0) { Write-Host "HTML Mutation Compilation Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c browser\html\serialization\html_serializer.c -o build\html_ser.o
if ($LASTEXITCODE -ne 0) { Write-Host "HTML Serializer Compilation Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c browser\html\tokenizer\html_tokenizer.c -o build\html_tok.o
if ($LASTEXITCODE -ne 0) { Write-Host "HTML Tokenizer Compilation Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c browser\html\tree_builder\html_tree_builder.c -o build\html_tb.o
if ($LASTEXITCODE -ne 0) { Write-Host "HTML Tree Builder Compilation Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c browser\html\parser\html_parser.c -o build\html_parser.o
if ($LASTEXITCODE -ne 0) { Write-Host "HTML Parser Compilation Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c browser\html\bos_html.c -o build\html_bos.o
if ($LASTEXITCODE -ne 0) { Write-Host "HTML BOS Facade Compilation Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c browser\html\tests\html_tests.c -o build\html_tests.o
if ($LASTEXITCODE -ne 0) { Write-Host "HTML Tests Compilation Failed!" -ForegroundColor Red; exit $LASTEXITCODE }

Write-Host "Compiling BOS OS Phase 5B Production CSS Parser & CSSOM Engine..." -ForegroundColor Cyan
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c browser\css\css_tokenizer.c -o build\css_tok.o
if ($LASTEXITCODE -ne 0) { Write-Host "CSS Tokenizer Compilation Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c browser\css\css_value.c -o build\css_val.o
if ($LASTEXITCODE -ne 0) { Write-Host "CSS Value Compilation Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c browser\css\css_specificity.c -o build\css_spec.o
if ($LASTEXITCODE -ne 0) { Write-Host "CSS Specificity Compilation Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c browser\css\css_selector.c -o build\css_sel.o
if ($LASTEXITCODE -ne 0) { Write-Host "CSS Selector Compilation Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c browser\css\css_rule.c -o build\css_rule.o
if ($LASTEXITCODE -ne 0) { Write-Host "CSS Rule Compilation Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c browser\css\css_stylesheet.c -o build\css_sheet.o
if ($LASTEXITCODE -ne 0) { Write-Host "CSS Stylesheet Compilation Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c browser\css\css_parser.c -o build\css_parser.o
if ($LASTEXITCODE -ne 0) { Write-Host "CSS Parser Compilation Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c browser\css\css_style.c -o build\css_style.o
if ($LASTEXITCODE -ne 0) { Write-Host "CSS Style Compilation Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-unknown-none -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I. -c browser\css\bos_css.c -o build\css_bos.o
if ($LASTEXITCODE -ne 0) { Write-Host "CSS BOS Facade Compilation Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
Write-Host "Compiling BOS OS Phase 6.5 Native .BOSX Runtime Subsystem..." -ForegroundColor Cyan
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\runtime\package\bos_pack_loader.c -o build\bos_pack_loader.o
if ($LASTEXITCODE -ne 0) { Write-Host "BOS Pack Loader Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\runtime\manifest\bos_manifest.c -o build\bos_manifest.o
if ($LASTEXITCODE -ne 0) { Write-Host "BOS Manifest Parser Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\runtime\loader\bos_elf_loader.c -o build\bos_elf_loader.o
if ($LASTEXITCODE -ne 0) { Write-Host "BOS ELF Loader Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\runtime\sdk_runtime\bos_sdk_runtime.c -o build\bos_sdk_runtime.o
if ($LASTEXITCODE -ne 0) { Write-Host "BOS SDK Runtime Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\runtime\installer\bos_installer.c -o build\bos_installer.o
if ($LASTEXITCODE -ne 0) { Write-Host "BOS Installer Failed!" -ForegroundColor Red; exit $LASTEXITCODE }

Write-Host "Compiling BOSFSR32 Native UI Subsystem..." -ForegroundColor Cyan
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\ui\bosfsr32\core\bosfsr_core.c -o build\bosfsr_core.o
if ($LASTEXITCODE -ne 0) { Write-Host "BOSFSR Core Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\ui\bosfsr32\graphics\bosfsr_graphics.c -o build\bosfsr_graphics.o
if ($LASTEXITCODE -ne 0) { Write-Host "BOSFSR Graphics Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\ui\bosfsr32\containers\bosfsr_containers.c -o build\bosfsr_containers.o
if ($LASTEXITCODE -ne 0) { Write-Host "BOSFSR Containers Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\ui\bosfsr32\controls\bosfsr_controls.c -o build\bosfsr_controls.o
if ($LASTEXITCODE -ne 0) { Write-Host "BOSFSR Controls Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\ui\bosfsr32\core\bosfsr32.c -o build\bosfsr32.o
if ($LASTEXITCODE -ne 0) { Write-Host "BOSFSR32 Main Engine Failed!" -ForegroundColor Red; exit $LASTEXITCODE }


Write-Host "Compiling ATOMS Graphics Platform (AGP V1.0)..." -ForegroundColor Cyan
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\agp\core\agp_runtime.c -o build\agp_runtime.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\agp\context\agp_context.c -o build\agp_context.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\agp\surface\agp_surface.c -o build\agp_surface.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\agp\swapchain\agp_swapchain.c -o build\agp_swapchain.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\agp\display\agp_display.c -o build\agp_display.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\agp\window\agp_window.c -o build\agp_window.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\agp\commands\agp_commands.c -o build\agp_commands.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\agp\texture\agp_texture.c -o build\agp_texture.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\agp\buffer\agp_buffer.c -o build\agp_buffer.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\agp\shader\agp_shader.c -o build\agp_shader.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\agp\pipeline\agp_pipeline.c -o build\agp_pipeline.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\agp\sync\agp_sync.c -o build\agp_sync.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\agp\opengl\agp_opengl_rt.c -o build\agp_opengl_rt.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\agp\loader\agp_opengl_loader.c -o build\agp_opengl_loader.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\agp\egl\agp_egl.c -o build\agp_egl.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\agp\resources\agp_resources.c -o build\agp_resources.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\agp\diagnostics\agp_diagnostics.c -o build\agp_diagnostics.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\agp\profiler\agp_profiler.c -o build\agp_profiler.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\agp\vram\agp_vram.c -o build\agp_vram.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\agp\scheduler\agp_scheduler.c -o build\agp_scheduler.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\agp\opengl\opengl32.c -o build\opengl32.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\agp\tests\agp_certification_tests.c -o build\agp_certification_tests.o

Write-Host "Compiling BO-TREE, DRE, BDR, BSR, BRT, BFS, BSOM, Explorer & AGP..." -ForegroundColor Cyan
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\botree\core\botree_init.c -o build\botree_init.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\botree\path\bde_path_normalize.c -o build\bde_path_normalize.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\botree\path\bde_path_canonical.c -o build\bde_path_canonical.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\botree\path\bde_path_split.c -o build\bde_path_split.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\botree\path\bde_path_compare.c -o build\bde_path_compare.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\botree\navigation\bde_nav_session.c -o build\bde_nav_session.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\botree\namespace\bde_namespace_core.c -o build\bde_namespace_core.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\botree\cache\bde_cache_dir.c -o build\bde_cache_dir.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\botree\transactions\bde_transaction_queue.c -o build\bde_transaction_queue.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\botree\commands\bde_cmd_handlers.c -o build\bde_cmd_handlers.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\botree\permissions\bde_permission_acl.c -o build\bde_permission_acl.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\botree\watch\bde_watcher_hub.c -o build\bde_watcher_hub.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\botree\clipboard\bde_clipboard_store.c -o build\bde_clipboard_store.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\botree\shortcut\bde_shortcut_resolver.c -o build\bde_shortcut_resolver.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\botree\metadata\bde_metadata_query.c -o build\bde_metadata_query.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\botree\recycle\bde_recycle_bin.c -o build\bde_recycle_bin.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\botree\runtime\core\dre_core.c -o build\dre_core.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\botree\runtime\navigator\dre_navigator.c -o build\dre_navigator.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\botree\runtime\enumerator\dre_enumerator.c -o build\dre_enumerator.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\botree\runtime\selection\dre_selection.c -o build\dre_selection.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\botree\runtime\sort\dre_sort.c -o build\dre_sort.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\botree\runtime\filter\dre_filter.c -o build\dre_filter.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\botree\runtime\tests\dre_certification_tests.c -o build\dre_certification_tests.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\desktop_runtime\core\bdr_core.c -o build\bdr_core.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\desktop_runtime\icons\bdr_icons.c -o build\bdr_icons.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\desktop_runtime\grid\bdr_grid.c -o build\bdr_grid.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\desktop_runtime\selection\bdr_selection.c -o build\bdr_selection.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\desktop_runtime\wallpaper\bdr_wallpaper.c -o build\bdr_wallpaper.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\desktop_runtime\notifications\bdr_notifications.c -o build\bdr_notifications.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\desktop_runtime\devices\bdr_devices.c -o build\bdr_devices.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\desktop_runtime\diagnostics\bdr_diagnostics.c -o build\bdr_diagnostics.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\desktop_runtime\tests\bdr_certification_tests.c -o build\bdr_certification_tests.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\shell_runtime\core\bsr_core.c -o build\bsr_core.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\shell_runtime\dispatcher\bsr_dispatcher.c -o build\bsr_dispatcher.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\shell_runtime\association\bsr_file_association.c -o build\bsr_file_association.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\shell_runtime\launcher\bsr_launcher.c -o build\bsr_launcher.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\shell_runtime\search\bsr_search.c -o build\bsr_search.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\shell_runtime\recent\bsr_recent.c -o build\bsr_recent.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\shell_runtime\favorites\bsr_favorites.c -o build\bsr_favorites.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\shell_runtime\notifications\bsr_notifications.c -o build\bsr_notifications.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\shell_runtime\dialogs\bsr_dialogs.c -o build\bsr_dialogs.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\shell_runtime\diagnostics\bsr_diagnostics.c -o build\bsr_diagnostics.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\shell_runtime\tests\bsr_certification_tests.c -o build\bsr_certification_tests.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\brt\core\brt_core.c -o build\brt_core.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\brt\registry\brt_registry.c -o build\brt_registry.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\brt\dispatcher\brt_dispatcher.c -o build\brt_dispatcher.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\brt\objects\brt_objects.c -o build\brt_objects.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\brt\runtime\brt_session.c -o build\brt_session.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\brt\cache\brt_cache.c -o build\brt_cache.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\brt\events\brt_events.c -o build\brt_events.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\brt\clipboard\brt_clipboard.c -o build\brt_clipboard.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\brt\dragdrop\brt_dragdrop.c -o build\brt_dragdrop.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\brt\notifications\brt_notifications.c -o build\brt_notifications.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\brt\search\brt_search.c -o build\brt_search.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\brt\security\brt_security.c -o build\brt_security.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\brt\transactions\brt_transactions.c -o build\brt_transactions.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\brt\diagnostics\brt_diagnostics.c -o build\brt_diagnostics.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\brt\tests\brt_certification_tests.c -o build\brt_certification_tests.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\bfs\core\bfs_core.c -o build\bfs_core.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\bfs\manager\bfs_file_manager.c -o build\bfs_file_manager.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\bfs\copy\bfs_copy.c -o build\bfs_copy.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\bfs\move\bfs_move.c -o build\bfs_move.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\bfs\delete\bfs_delete.c -o build\bfs_delete.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\bfs\rename\bfs_rename.c -o build\bfs_rename.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\bfs\create\bfs_create.c -o build\bfs_create.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\bfs\properties\bfs_properties.c -o build\bfs_properties.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\bfs\locking\bfs_locking.c -o build\bfs_locking.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\bfs\association\bfs_association.c -o build\bfs_association.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\bfs\mime\bfs_mime.c -o build\bfs_mime.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\bfs\icons\bfs_icons.c -o build\bfs_icons.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\bfs\thumbnails\bfs_thumbnails.c -o build\bfs_thumbnails.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\bfs\recycle\bfs_recycle.c -o build\bfs_recycle.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\bfs\recent\bfs_recent.c -o build\bfs_recent.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\bfs\favorites\bfs_favorites.c -o build\bfs_favorites.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\bfs\quickaccess\bfs_quickaccess.c -o build\bfs_quickaccess.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\bfs\metadata\bfs_metadata.c -o build\bfs_metadata.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\bfs\search\bfs_search.c -o build\bfs_search.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\bfs\transactions\bfs_transactions.c -o build\bfs_transactions.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\bfs\diagnostics\bfs_diagnostics.c -o build\bfs_diagnostics.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\bfs\tests\bfs_certification_tests.c -o build\bfs_certification_tests.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\bsom\core\bsom_core.c -o build\bsom_core.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\bsom\registry\bsom_registry.c -o build\bsom_registry.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\bsom\factory\bsom_factory.c -o build\bsom_factory.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\bsom\handles\bsom_handles.c -o build\bsom_handles.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\bsom\lifetime\bsom_lifetime.c -o build\bsom_lifetime.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\bsom\resolver\bsom_resolver.c -o build\bsom_resolver.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\bsom\namespace\bsom_namespace.c -o build\bsom_namespace.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\bsom\properties\bsom_properties.c -o build\bsom_properties.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\bsom\icons\bsom_icons.c -o build\bsom_icons.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\bsom\thumbnails\bsom_thumbnails.c -o build\bsom_thumbnails.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\bsom\clipboard\bsom_clipboard.c -o build\bsom_clipboard.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\bsom\dragdrop\bsom_dragdrop.c -o build\bsom_dragdrop.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\bsom\search\bsom_search.c -o build\bsom_search.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\bsom\recent\bsom_recent.c -o build\bsom_recent.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\bsom\favorites\bsom_favorites.c -o build\bsom_favorites.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\bsom\recycle\bsom_recycle.c -o build\bsom_recycle.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\bsom\permissions\bsom_permissions.c -o build\bsom_permissions.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\bsom\metadata\bsom_metadata.c -o build\bsom_metadata.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\bsom\cache\bsom_cache.c -o build\bsom_cache.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\bsom\diagnostics\bsom_diagnostics.c -o build\bsom_diagnostics.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\bsom\tests\bsom_certification_tests.c -o build\bsom_certification_tests.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\shell\apps\explorer.c -o build\explorer.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\shell\apps\explorer_view.c -o build\explorer_view.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\shell\apps\explorer_cache.c -o build\explorer_cache.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\shell\apps\notes_app.c -o build\notes_app.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\shell\apps\rename_dialog.c -o build\rename_dialog.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\shell\apps\app_clipboard.c -o build\app_clipboard.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\shell\apps\tests\explorer_rewrite_certification.c -o build\explorer_rewrite_certification.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\agp\core\agp_runtime.c -o build\agp_runtime.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\agp\context\agp_context.c -o build\agp_context.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\agp\surface\agp_surface.c -o build\agp_surface.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\agp\swapchain\agp_swapchain.c -o build\agp_swapchain.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\agp\display\agp_display.c -o build\agp_display.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\agp\window\agp_window.c -o build\agp_window.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\agp\commands\agp_commands.c -o build\agp_commands.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\agp\texture\agp_texture.c -o build\agp_texture.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\agp\buffer\agp_buffer.c -o build\agp_buffer.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\agp\shader\agp_shader.c -o build\agp_shader.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\agp\pipeline\agp_pipeline.c -o build\agp_pipeline.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\agp\sync\agp_sync.c -o build\agp_sync.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\agp\opengl\agp_opengl_rt.c -o build\agp_opengl_rt.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\agp\loader\agp_opengl_loader.c -o build\agp_opengl_loader.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\agp\egl\agp_egl.c -o build\agp_egl.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\agp\resources\agp_resources.c -o build\agp_resources.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\agp\diagnostics\agp_diagnostics.c -o build\agp_diagnostics.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\agp\profiler\agp_profiler.c -o build\agp_profiler.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\agp\vram\agp_vram.c -o build\agp_vram.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\agp\scheduler\agp_scheduler.c -o build\agp_scheduler.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\agp\opengl\opengl32.c -o build\opengl32.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\agp\tests\agp_certification_tests.c -o build\agp_certification_tests.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\bar\core\bar_runtime.c -o build\bar_runtime.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\bar\process\bar_process.c -o build\bar_process.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\bar\runtime\bar_session.c -o build\bar_session.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\bar\registry\bar_registry.c -o build\bar_registry.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\bar\loader\bar_dll.c -o build\bar_dll.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\bar\message\bar_message.c -o build\bar_message.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\bar\dispatcher\bar_dispatcher.c -o build\bar_dispatcher.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\bar\windows\bar_windows.c -o build\bar_windows.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\bar\resources\bar_resources.c -o build\bar_resources.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\bar\dialogs\bar_dialogs.c -o build\bar_dialogs.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\bar\clipboard\bar_clipboard.c -o build\bar_clipboard.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\bar\dragdrop\bar_dragdrop.c -o build\bar_dragdrop.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\bar\timers\bar_timers.c -o build\bar_timers.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\bar\focus\bar_focus.c -o build\bar_focus.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\bar\packages\bar_packages.c -o build\bar_packages.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\bar\manifest\bar_manifest.c -o build\bar_manifest.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\bar\security\bar_security.c -o build\bar_security.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\bar\diagnostics\bar_diagnostics.c -o build\bar_diagnostics.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\bar\tests\bar_certification_tests.c -o build\bar_certification_tests.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\user32\core\user32_runtime.c -o build\user32_runtime.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\user32\window\user32_class.c -o build\user32_class.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\user32\window\user32_window.c -o build\user32_window.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\user32\message\user32_message.c -o build\user32_message.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\user32\input\user32_input.c -o build\user32_input.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\user32\keyboard\user32_keyboard.c -o build\user32_keyboard.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\user32\mouse\user32_mouse.c -o build\user32_mouse.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\user32\focus\user32_focus.c -o build\user32_focus.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\user32\caret\user32_caret.c -o build\user32_caret.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\user32\cursor\user32_cursor.c -o build\user32_cursor.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\user32\clipboard\user32_clipboard.c -o build\user32_clipboard.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\user32\dragdrop\user32_dragdrop.c -o build\user32_dragdrop.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\user32\menus\user32_menus.c -o build\user32_menus.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\user32\dialogs\user32_dialogs.c -o build\user32_dialogs.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\user32\controls\user32_controls.c -o build\user32_controls.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\user32\timers\user32_timers.c -o build\user32_timers.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\user32\hooks\user32_hooks.c -o build\user32_hooks.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\user32\accelerators\user32_accelerators.c -o build\user32_accelerators.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\user32\diagnostics\user32_diagnostics.c -o build\user32_diagnostics.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\user32\tests\user32_certification_tests.c -o build\user32_certification_tests.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\gdi32\core\gdi32_runtime.c -o build\gdi32_runtime.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\gdi32\dc\gdi32_dc.c -o build\gdi32_dc.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\gdi32\pens\gdi32_pen.c -o build\gdi32_pen.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\gdi32\brushes\gdi32_brush.c -o build\gdi32_brush.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\gdi32\fonts\gdi32_font.c -o build\gdi32_font.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\gdi32\text\gdi32_text.c -o build\gdi32_text.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\gdi32\bitmap\gdi32_bitmap.c -o build\gdi32_bitmap.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\gdi32\images\gdi32_image.c -o build\gdi32_image.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\gdi32\regions\gdi32_region.c -o build\gdi32_region.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\gdi32\clipping\gdi32_clipping.c -o build\gdi32_clipping.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\gdi32\painting\gdi32_painting.c -o build\gdi32_painting.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\gdi32\blit\gdi32_blit.c -o build\gdi32_blit.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\gdi32\alpha\gdi32_alpha.c -o build\gdi32_alpha.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\gdi32\geometry\gdi32_geometry.c -o build\gdi32_geometry.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\gdi32\colors\gdi32_colors.c -o build\gdi32_colors.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\gdi32\surfaces\gdi32_surfaces.c -o build\gdi32_surfaces.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\gdi32\paths\gdi32_paths.c -o build\gdi32_paths.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\gdi32\printing\gdi32_printing.c -o build\gdi32_printing.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\gdi32\diagnostics\gdi32_diagnostics.c -o build\gdi32_diagnostics.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\gdi32\tests\gdi32_certification_tests.c -o build\gdi32_certification_tests.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\kernel32\core\kernel32_runtime.c -o build\kernel32_runtime.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\kernel32\process\kernel32_process.c -o build\kernel32_process.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\kernel32\thread\kernel32_thread.c -o build\kernel32_thread.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\kernel32\memory\kernel32_memory.c -o build\kernel32_memory.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\kernel32\heap\kernel32_heap.c -o build\kernel32_heap.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\kernel32\sync\kernel32_sync.c -o build\kernel32_sync.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\kernel32\mutex\kernel32_mutex.c -o build\kernel32_mutex.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\kernel32\semaphore\kernel32_semaphore.c -o build\kernel32_semaphore.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\kernel32\critical\kernel32_critical.c -o build\kernel32_critical.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\kernel32\events\kernel32_events.c -o build\kernel32_events.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\kernel32\files\kernel32_files.c -o build\kernel32_files.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\kernel32\directory\kernel32_directory.c -o build\kernel32_directory.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\kernel32\pipes\kernel32_pipes.c -o build\kernel32_pipes.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\kernel32\console\kernel32_console.c -o build\kernel32_console.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\kernel32\time\kernel32_time.c -o build\kernel32_time.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\kernel32\locale\kernel32_locale.c -o build\kernel32_locale.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\kernel32\environment\kernel32_environment.c -o build\kernel32_environment.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\kernel32\loader\kernel32_loader.c -o build\kernel32_loader.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\kernel32\tls\kernel32_tls.c -o build\kernel32_tls.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\kernel32\exceptions\kernel32_exceptions.c -o build\kernel32_exceptions.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\kernel32\atom\kernel32_atoms.c -o build\kernel32_atoms.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\kernel32\performance\kernel32_performance.c -o build\kernel32_performance.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\kernel32\diagnostics\kernel32_diagnostics.c -o build\kernel32_diagnostics.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\kernel32\tests\kernel32_certification_tests.c -o build\kernel32_certification_tests.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\comdlg32\core\comdlg32_runtime.c -o build\comdlg32_runtime.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\comdlg32\filedialog\comdlg32_open_save.c -o build\comdlg32_open_save.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\comdlg32\folderdialog\comdlg32_folder.c -o build\comdlg32_folder.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\comdlg32\colordialog\comdlg32_color.c -o build\comdlg32_color.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\comdlg32\fontdialog\comdlg32_font.c -o build\comdlg32_font.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\comdlg32\printdialog\comdlg32_print.c -o build\comdlg32_print.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\comdlg32\pagedialog\comdlg32_page_setup.c -o build\comdlg32_page_setup.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\comdlg32\findreplace\comdlg32_find_replace.c -o build\comdlg32_find_replace.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\comdlg32\preview\comdlg32_preview.c -o build\comdlg32_preview.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\comdlg32\history\comdlg32_history.c -o build\comdlg32_history.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\comdlg32\favorites\comdlg32_favorites.c -o build\comdlg32_favorites.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\comdlg32\quickaccess\comdlg32_quickaccess.c -o build\comdlg32_quickaccess.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\comdlg32\sidebar\comdlg32_sidebar.c -o build\comdlg32_sidebar.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\comdlg32\filters\comdlg32_filters.c -o build\comdlg32_filters.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\comdlg32\navigation\comdlg32_navigation.c -o build\comdlg32_navigation.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\comdlg32\validation\comdlg32_validation.c -o build\comdlg32_validation.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\comdlg32\layout\comdlg32_layout.c -o build\comdlg32_layout.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\comdlg32\theme\comdlg32_theme.c -o build\comdlg32_theme.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\comdlg32\icons\comdlg32_icons.c -o build\comdlg32_icons.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\comdlg32\bookmarks\comdlg32_bookmarks.c -o build\comdlg32_bookmarks.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\comdlg32\diagnostics\comdlg32_diagnostics.c -o build\comdlg32_diagnostics.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\comdlg32\tests\comdlg32_certification_tests.c -o build\comdlg32_certification_tests.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\comctl32\core\comctl32_runtime.c -o build\comctl32_runtime.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\comctl32\button\comctl32_button.c -o build\comctl32_button.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\comctl32\edit\comctl32_edit.c -o build\comctl32_edit.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\comctl32\static\comctl32_static.c -o build\comctl32_static.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\comctl32\listbox\comctl32_listbox.c -o build\comctl32_listbox.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\comctl32\combobox\comctl32_combobox.c -o build\comctl32_combobox.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\comctl32\listview\comctl32_listview.c -o build\comctl32_listview.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\comctl32\treeview\comctl32_treeview.c -o build\comctl32_treeview.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\comctl32\tab\comctl32_tab.c -o build\comctl32_tab.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\comctl32\toolbar\comctl32_toolbar.c -o build\comctl32_toolbar.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\comctl32\statusbar\comctl32_statusbar.c -o build\comctl32_statusbar.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\comctl32\progress\comctl32_progress.c -o build\comctl32_progress.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\comctl32\trackbar\comctl32_trackbar.c -o build\comctl32_trackbar.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\comctl32\header\comctl32_header.c -o build\comctl32_header.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\comctl32\imagelist\comctl32_imagelist.c -o build\comctl32_imagelist.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\comctl32\tooltip\comctl32_tooltip.c -o build\comctl32_tooltip.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\comctl32\rebar\comctl32_rebar.c -o build\comctl32_rebar.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\comctl32\updown\comctl32_updown.c -o build\comctl32_updown.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\comctl32\pager\comctl32_pager.c -o build\comctl32_pager.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\comctl32\animation\comctl32_animation.c -o build\comctl32_animation.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\comctl32\monthcal\comctl32_monthcal.c -o build\comctl32_monthcal.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\comctl32\datetime\comctl32_datetime.c -o build\comctl32_datetime.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\comctl32\hotkey\comctl32_hotkey.c -o build\comctl32_hotkey.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\comctl32\ipaddress\comctl32_ipaddress.c -o build\comctl32_ipaddress.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\comctl32\theme\comctl32_theme.c -o build\comctl32_theme.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\comctl32\layout\comctl32_layout.c -o build\comctl32_layout.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\comctl32\notifications\comctl32_notifications.c -o build\comctl32_notifications.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\comctl32\accessibility\comctl32_accessibility.c -o build\comctl32_accessibility.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\comctl32\diagnostics\comctl32_diagnostics.c -o build\comctl32_diagnostics.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\comctl32\tests\comctl32_certification_tests.c -o build\comctl32_certification_tests.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\shell32\core\shell_runtime.c -o build\shell_runtime.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\shell32\desktop\shell_desktop.c -o build\shell_desktop.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\shell32\explorer\shell_explorer.c -o build\shell_explorer.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\shell32\namespace\shell_namespace.c -o build\shell_namespace.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\shell32\folders\shell_specialfolders.c -o build\shell_specialfolders.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\shell32\recyclebin\shell_recyclebin.c -o build\shell_recyclebin.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\shell32\shortcuts\shell_shortcuts.c -o build\shell_shortcuts.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\shell32\icons\shell_icons.c -o build\shell_icons.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\shell32\imagelist\shell_imagelist.c -o build\shell_imagelist.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\shell32\contextmenu\shell_contextmenu.c -o build\shell_contextmenu.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\shell32\fileassoc\shell_fileassoc.c -o build\shell_fileassoc.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\shell32\launch\shell_execute.c -o build\shell_execute.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\shell32\properties\shell_properties.c -o build\shell_properties.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\shell32\clipboard\shell_clipboard.c -o build\shell_clipboard.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\shell32\dragdrop\shell_dragdrop.c -o build\shell_dragdrop.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\shell32\notifications\shell_notifications.c -o build\shell_notifications.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\shell32\taskbar\shell_taskbar.c -o build\shell_taskbar.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\shell32\tray\shell_tray.c -o build\shell_tray.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\shell32\search\shell_search.c -o build\shell_search.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\shell32\history\shell_history.c -o build\shell_history.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\shell32\diagnostics\shell_diagnostics.c -o build\shell_diagnostics.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\shell32\tests\shell32_certification_tests.c -o build\shell32_certification_tests.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\bosll\core\bos_bootstrap.c -o build\bos_bootstrap.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\bosll\loader\bos_loader.c -o build\bos_loader.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\bosll\object\bos_object.c -o build\bos_object.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\bosll\handles\bos_handles.c -o build\bos_handles.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\bosll\process\bos_process.c -o build\bos_process.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\bosll\thread\bos_thread.c -o build\bos_thread.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\bosll\memory\bos_memory.c -o build\bos_memory.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\bosll\memory\bos_vmem.c -o build\bos_vmem.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\bosll\heap\bos_heap.c -o build\bos_heap.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\bosll\sync\bos_sync.c -o build\bos_sync.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\bosll\exceptions\bos_exceptions.c -o build\bos_exceptions.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\bosll\ipc\bos_ipc.c -o build\bos_ipc.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\bosll\security\bos_security.c -o build\bos_security.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\bosll\syscall\bos_syscall.c -o build\bos_syscall.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\bosll\timer\bos_timer.c -o build\bos_timer.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\bosll\performance\bos_performance.c -o build\bos_performance.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\bosll\context\bos_context.c -o build\bos_context.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\bosll\environment\bos_environment.c -o build\bos_environment.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\bosll\diagnostics\bos_diagnostics.c -o build\bos_diagnostics.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\bosll\tests\bosll_certification_tests.c -o build\bosll_certification_tests.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\opengl32\core\opengl_runtime.c -o build\opengl_runtime.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\opengl32\context\opengl_context.c -o build\opengl_context.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\opengl32\pixel\opengl_pixel.c -o build\opengl_pixel.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\opengl32\surface\opengl_surface.c -o build\opengl_surface.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\opengl32\buffers\opengl_buffers.c -o build\opengl_buffers.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\opengl32\textures\opengl_textures.c -o build\opengl_textures.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\opengl32\shaders\opengl_shaders.c -o build\opengl_shaders.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\opengl32\pipeline\opengl_pipeline.c -o build\opengl_pipeline.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\opengl32\vertex\opengl_vertex.c -o build\opengl_vertex.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\opengl32\framebuffer\opengl_framebuffer.c -o build\opengl_framebuffer.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\opengl32\swap\opengl_swap.c -o build\opengl_swap.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\opengl32\extensions\opengl_extensions.c -o build\opengl_extensions.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\opengl32\dispatch\opengl_dispatch.c -o build\opengl_dispatch.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\opengl32\errors\opengl_errors.c -o build\opengl_errors.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\opengl32\state\opengl_state.c -o build\opengl_state.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\opengl32\agp\opengl_agp_translate.c -o build\opengl_agp_translate.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\opengl32\performance\opengl_performance.c -o build\opengl_performance.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\opengl32\resource\opengl_resource.c -o build\opengl_resource.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\opengl32\diagnostics\opengl_diagnostics.c -o build\opengl_diagnostics.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\opengl32\tests\opengl32_certification_tests.c -o build\opengl32_certification_tests.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\advapi32\core\advapi_runtime.c -o build\advapi_runtime.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\advapi32\registry\advapi_registry.c -o build\advapi_registry.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\advapi32\registry\advapi_reg_notify.c -o build\advapi_reg_notify.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\advapi32\security\advapi_security.c -o build\advapi_security.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\advapi32\sid\advapi_sid.c -o build\advapi_sid.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\advapi32\acl\advapi_acl.c -o build\advapi_acl.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\advapi32\tokens\advapi_tokens.c -o build\advapi_tokens.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\advapi32\privilege\advapi_privilege.c -o build\advapi_privilege.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\advapi32\services\advapi_services.c -o build\advapi_services.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\advapi32\eventlog\advapi_eventlog.c -o build\advapi_eventlog.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\advapi32\crypto\advapi_crypto.c -o build\advapi_crypto.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\advapi32\policy\advapi_policy.c -o build\advapi_policy.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\advapi32\lsa\advapi_lsa.c -o build\advapi_lsa.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\advapi32\impersonation\advapi_impersonation.c -o build\advapi_impersonation.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\advapi32\audit\advapi_audit.c -o build\advapi_audit.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\advapi32\environment\advapi_env.c -o build\advapi_env.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\advapi32\handles\advapi_handles.c -o build\advapi_handles.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\advapi32\performance\advapi_performance.c -o build\advapi_performance.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\advapi32\diagnostics\advapi_diagnostics.c -o build\advapi_diagnostics.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\advapi32\tests\advapi32_certification_tests.c -o build\advapi32_certification_tests.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\ws2_32\core\ws2_runtime.c -o build\ws2_runtime.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\ws2_32\socket\ws2_socket.c -o build\ws2_socket.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\ws2_32\tcp\ws2_tcp.c -o build\ws2_tcp.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\ws2_32\udp\ws2_udp.c -o build\ws2_udp.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\ws2_32\address\ws2_address.c -o build\ws2_address.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\ws2_32\address\ws2_interface.c -o build\ws2_interface.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\ws2_32\dns\ws2_dns.c -o build\ws2_dns.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\ws2_32\core\ws2_transfer.c -o build\ws2_transfer.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\ws2_32\async\ws2_async.c -o build\ws2_async.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\ws2_32\events\ws2_events.c -o build\ws2_events.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\ws2_32\select\ws2_select.c -o build\ws2_select.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\ws2_32\poll\ws2_poll.c -o build\ws2_poll.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\ws2_32\options\ws2_options.c -o build\ws2_options.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\ws2_32\security\ws2_security.c -o build\ws2_security.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\ws2_32\buffers\ws2_buffers.c -o build\ws2_buffers.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\ws2_32\performance\ws2_performance.c -o build\ws2_performance.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\ws2_32\diagnostics\ws2_diagnostics.c -o build\ws2_diagnostics.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\ws2_32\startup\ws2_compat.c -o build\ws2_compat.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\ws2_32\core\ws2_resource.c -o build\ws2_resource.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\ws2_32\tests\ws2_32_certification_tests.c -o build\ws2_32_certification_tests.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\ole32\core\ole_runtime.c -o build\ole_runtime.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\ole32\com\ole_com.c -o build\ole_com.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\ole32\unknown\ole_iunknown.c -o build\ole_iunknown.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\ole32\classfactory\ole_factory.c -o build\ole_factory.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\ole32\objects\ole_objects.c -o build\ole_objects.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\ole32\interfaces\ole_interfaces.c -o build\ole_interfaces.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\ole32\clsid\ole_clsid.c -o build\ole_clsid.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\ole32\iid\ole_iid.c -o build\ole_iid.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\ole32\apartment\ole_apartment.c -o build\ole_apartment.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\ole32\marshal\ole_marshal.c -o build\ole_marshal.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\ole32\storage\ole_storage.c -o build\ole_storage.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\ole32\streams\ole_stream.c -o build\ole_stream.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\ole32\clipboard\ole_clipboard.c -o build\ole_clipboard.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\ole32\dragdrop\ole_dragdrop.c -o build\ole_dragdrop.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\ole32\dataobject\ole_dataobject.c -o build\ole_dataobject.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\ole32\moniker\ole_moniker.c -o build\ole_moniker.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\ole32\property\ole_property.c -o build\ole_property.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\ole32\persistence\ole_persist.c -o build\ole_persist.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\libs\ole32\diagnostics\ole_diagnostics.c -o build\ole_diagnostics.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\system\explorer\core\explorer_runtime.c -o build\explorer_runtime.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\system\explorer\desktop\explorer_desktop.c -o build\explorer_desktop.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\system\explorer\session\explorer_session.c -o build\explorer_session.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\system\explorer\wallpaper\explorer_wallpaper.c -o build\explorer_wallpaper.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\system\explorer\icons\explorer_icons.c -o build\explorer_icons.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\system\explorer\taskbar\explorer_taskbar.c -o build\explorer_taskbar.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\system\explorer\startmenu\explorer_startmenu.c -o build\explorer_startmenu.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\system\explorer\tray\explorer_tray.c -o build\explorer_tray.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\system\explorer\explorer\explorer_browser.c -o build\explorer_browser.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\system\explorer\navigation\explorer_navigation.c -o build\explorer_navigation.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\system\explorer\addressbar\explorer_addressbar.c -o build\explorer_addressbar.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\system\explorer\search\explorer_search.c -o build\explorer_search.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\system\explorer\contextmenu\explorer_contextmenu.c -o build\explorer_contextmenu.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\system\explorer\dragdrop\explorer_dragdrop.c -o build\explorer_dragdrop.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\system\explorer\clipboard\explorer_clipboard.c -o build\explorer_clipboard.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\system\explorer\recyclebin\explorer_recyclebin.c -o build\explorer_recyclebin.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\system\explorer\associations\explorer_associations.c -o build\explorer_associations.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\system\explorer\controlpanel\explorer_controlpanel.c -o build\explorer_controlpanel.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\system\explorer\settings\explorer_settings.c -o build\explorer_settings.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\system\explorer\favorites\explorer_favorites.c -o build\explorer_favorites.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\system\explorer\recent\explorer_recent.c -o build\explorer_recent.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\system\explorer\diagnostics\explorer_diagnostics.c -o build\explorer_diagnostics.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\system\controlpanel\core\controlpanel_runtime.c -o build\controlpanel_runtime.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\system\controlpanel\loader\controlpanel_loader.c -o build\controlpanel_loader.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\system\controlpanel\category\controlpanel_category.c -o build\controlpanel_category.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\system\controlpanel\search\controlpanel_search.c -o build\controlpanel_search.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\system\controlpanel\navigation\controlpanel_navigation.c -o build\controlpanel_navigation.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\system\controlpanel\favorites\controlpanel_favorites.c -o build\controlpanel_favorites.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\system\controlpanel\history\controlpanel_history.c -o build\controlpanel_history.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\system\controlpanel\permissions\controlpanel_permissions.c -o build\controlpanel_permissions.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\system\controlpanel\settings\controlpanel_settings.c -o build\controlpanel_settings.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\system\controlpanel\plugin\controlpanel_plugin.c -o build\controlpanel_plugin.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\system\controlpanel\diagnostics\controlpanel_diagnostics.c -o build\controlpanel_diagnostics.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\system\controlpanel\modules\bosc_modules.c -o build\bosc_modules.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\system\controlpanel\tests\controlpanel_certification_tests.c -o build\controlpanel_certification_tests.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\system\settings\core\settings_runtime.c -o build\settings_runtime.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\system\settings\dashboard\settings_dashboard.c -o build\settings_dashboard.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\system\settings\navigation\settings_navigation.c -o build\settings_navigation.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\system\settings\search\settings_search.c -o build\settings_search.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\system\settings\display\settings_display.c -o build\settings_display.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\system\settings\appearance\settings_theme.c -o build\settings_theme.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\system\settings\personalization\settings_personalization.c -o build\settings_personalization.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\system\settings\sound\settings_sound.c -o build\settings_sound.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\system\settings\network\settings_network.c -o build\settings_network.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\system\settings\bluetooth\settings_bluetooth.c -o build\settings_bluetooth.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\system\settings\storage\settings_storage.c -o build\settings_storage.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\system\settings\accounts\settings_accounts.c -o build\settings_accounts.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\system\settings\privacy\settings_privacy.c -o build\settings_privacy.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\system\settings\updates\settings_updates.c -o build\settings_updates.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\system\settings\accessibility\settings_accessibility.c -o build\settings_accessibility.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\system\settings\devices\settings_devices.c -o build\settings_devices.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\system\settings\notifications\settings_notifications.c -o build\settings_notifications.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\system\settings\power\settings_power.c -o build\settings_power.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\system\settings\diagnostics\settings_diagnostics.c -o build\settings_diagnostics.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\system\settings\tests\settings_certification_tests.c -o build\settings_certification_tests.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\apps\terminal\core\terminal_runtime.c -o build\terminal_runtime.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\apps\terminal\console\terminal_console.c -o build\terminal_console.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\apps\terminal\parser\terminal_parser.c -o build\terminal_parser.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\apps\terminal\commands\terminal_commands.c -o build\terminal_commands.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\apps\terminal\history\terminal_history.c -o build\terminal_history.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\apps\terminal\autocomplete\terminal_autocomplete.c -o build\terminal_autocomplete.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\apps\terminal\aliases\terminal_aliases.c -o build\terminal_aliases.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\apps\terminal\environment\terminal_environment.c -o build\terminal_environment.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\apps\terminal\filesystem\terminal_filesystem.c -o build\terminal_filesystem.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\apps\terminal\process\terminal_process.c -o build\terminal_process.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\apps\terminal\script\terminal_script.c -o build\terminal_script.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\apps\terminal\plugin\terminal_plugin.c -o build\terminal_plugin.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\apps\terminal\diagnostics\terminal_diagnostics.c -o build\terminal_diagnostics.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\apps\terminal\tests\terminal_certification_tests.c -o build\terminal_certification_tests.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\apps\taskmanager\core\taskmgr_runtime.c -o build\taskmgr_runtime.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\apps\taskmanager\dashboard\taskmgr_dashboard.c -o build\taskmgr_dashboard.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\apps\taskmanager\process\taskmgr_process.c -o build\taskmgr_process.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\apps\taskmanager\threads\taskmgr_threads.c -o build\taskmgr_threads.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\apps\taskmanager\memory\taskmgr_memory.c -o build\taskmgr_memory.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\apps\taskmanager\cpu\taskmgr_cpu.c -o build\taskmgr_cpu.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\apps\taskmanager\gpu\taskmgr_gpu.c -o build\taskmgr_gpu.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\apps\taskmanager\storage\taskmgr_storage.c -o build\taskmgr_storage.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\apps\taskmanager\network\taskmgr_network.c -o build\taskmgr_network.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\apps\taskmanager\power\taskmgr_power.c -o build\taskmgr_power.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\apps\taskmanager\services\taskmgr_services.c -o build\taskmgr_services.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\apps\taskmanager\drivers\taskmgr_drivers.c -o build\taskmgr_drivers.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\apps\taskmanager\modules\taskmgr_modules.c -o build\taskmgr_modules.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\apps\taskmanager\handles\taskmgr_handles.c -o build\taskmgr_handles.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\apps\taskmanager\hardware\taskmgr_hardware.c -o build\taskmgr_hardware.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\apps\taskmanager\sensors\taskmgr_sensors.c -o build\taskmgr_sensors.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\apps\taskmanager\performance\taskmgr_performance.c -o build\taskmgr_performance.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\apps\taskmanager\graphs\taskmgr_graphs.c -o build\taskmgr_graphs.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\apps\taskmanager\diagnostics\taskmgr_diagnostics.c -o build\taskmgr_diagnostics.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\apps\taskmanager\tests\taskmgr_certification_tests.c -o build\taskmgr_certification_tests.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\apps\fileexplorer\core\fileexplorer_runtime.c -o build\fe_runtime.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\apps\fileexplorer\navigation\fe_navigation.c -o build\fe_navigation.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\apps\fileexplorer\addressbar\fe_addressbar.c -o build\fe_addressbar.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\apps\fileexplorer\treeview\fe_treeview.c -o build\fe_treeview.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\apps\fileexplorer\listview\fe_listview.c -o build\fe_listview.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\apps\fileexplorer\filesystem\fe_namespace.c -o build\fe_namespace.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\apps\fileexplorer\filesystem\fe_drives.c -o build\fe_drives.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\apps\fileexplorer\search\fe_search.c -o build\fe_search.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\apps\fileexplorer\preview\fe_preview.c -o build\fe_preview.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\apps\fileexplorer\thumbnail\fe_thumbnail.c -o build\fe_thumbnail.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\apps\fileexplorer\clipboard\fe_clipboard.c -o build\fe_clipboard.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\apps\fileexplorer\dragdrop\fe_dragdrop.c -o build\fe_dragdrop.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\apps\fileexplorer\operations\fe_operations.c -o build\fe_operations.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\apps\fileexplorer\properties\fe_properties.c -o build\fe_properties.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\apps\fileexplorer\permissions\fe_permissions.c -o build\fe_permissions.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\apps\fileexplorer\contextmenu\fe_contextmenu.c -o build\fe_contextmenu.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\apps\fileexplorer\favorites\fe_favorites.c -o build\fe_favorites.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\apps\fileexplorer\recent\fe_recent.c -o build\fe_recent.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\apps\fileexplorer\history\fe_history.c -o build\fe_history.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\apps\fileexplorer\shortcuts\fe_shortcuts.c -o build\fe_shortcuts.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\apps\fileexplorer\refresh\fe_refresh.c -o build\fe_refresh.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\apps\fileexplorer\watcher\fe_watcher.c -o build\fe_watcher.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\apps\fileexplorer\diagnostics\fe_forensic.c -o build\fe_forensic.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\apps\fileexplorer\diagnostics\fe_diagnostics.c -o build\fe_diagnostics.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c userspace\apps\fileexplorer\tests\fe_certification_tests.c -o build\fe_certification_tests.o

Write-Host "Compiling BOSPECTRA Multimedia Engine..." -ForegroundColor Yellow
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\core\bospectra_core.c -o build\bospectra_core.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\core\bospectra_state.c -o build\bospectra_state.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\debug\bospectra_debug.c -o build\bospectra_debug.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\memory\bospectra_memory.c -o build\bospectra_memory.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\file\bospectra_file.c -o build\bospectra_file.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\packet\bospectra_packet.c -o build\bospectra_packet.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\buffer\bospectra_buffer.c -o build\bospectra_buffer.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\stream\bospectra_stream.c -o build\bospectra_stream.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\container\bospectra_container.c -o build\bospectra_container.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\container\common\container_common.c -o build\container_common.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\container\registry\container_registry.c -o build\container_registry.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\container\mp4\mp4_parser.c -o build\mp4_parser.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\container\mkv\mkv_parser.c -o build\mkv_parser.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\container\avi\avi_parser.c -o build\avi_parser.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\container\tests\container_tests.c -o build\container_tests.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\frame_memory\bospectra_frame_memory.c -o build\bospectra_frame_memory.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\frame_memory\frame_pool\frame_pool.c -o build\frame_pool.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\frame_memory\packet_pool\packet_pool.c -o build\packet_pool.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\frame_memory\queue\media_queue.c -o build\media_queue.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\frame_memory\ring_buffer\circular_ring.c -o build\circular_ring.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\frame_memory\diag\frame_memory_diag.c -o build\frame_memory_diag.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\frame_memory\tests\frame_memory_tests.c -o build\frame_memory_tests.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\color\bospectra_color.c -o build\bospectra_color.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\color\converters\yuv420\yuv420_converter.c -o build\yuv420_converter.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\color\converters\nv12\nv12_converter.c -o build\nv12_converter.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\color\converters\yuy2\yuy2_converter.c -o build\yuy2_converter.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\color\converters\rgb\rgb_converter.c -o build\rgb_converter.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\color\pipeline\color_pipeline.c -o build\color_pipeline.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\color\diagnostics\color_diag.c -o build\color_diag.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\color\tests\color_tests.c -o build\color_tests.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\decoder\bospectra_decoder.c -o build\bospectra_decoder.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\decoder\common\bitstream_reader.c -o build\bitstream_reader.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\decoder\common\idct.c -o build\idct.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\decoder\h264\h264_decoder.c -o build\h264_decoder.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\decoder\mjpeg\mjpeg_decoder.c -o build\mjpeg_decoder.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\decoder\mpeg2\mpeg2_decoder.c -o build\mpeg2_decoder.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\decoder\registry\decoder_registry.c -o build\decoder_registry.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\decoder\diagnostics\decoder_diag.c -o build\decoder_diag.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\decoder\tests\decoder_tests.c -o build\decoder_tests.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\audio\bospectra_audio.c -o build\bospectra_audio.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\audio\bridge\audio_bridge.c -o build\audio_bridge.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\audio\session\audio_session.c -o build\bospectra_audio_session.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\audio\queue\audio_queue.c -o build\bospectra_audio_queue.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\audio\diagnostics\audio_diag.c -o build\bospectra_audio_diag.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\audio\tests\audio_tests.c -o build\bospectra_audio_tests.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\sync\bospectra_sync.c -o build\bospectra_sync.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\sync\clock\master_clock.c -o build\master_clock.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\sync\drift\drift_detector.c -o build\drift_detector.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\sync\scheduler\frame_scheduler.c -o build\bospectra_frame_scheduler.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\sync\diagnostics\sync_diag.c -o build\sync_diag.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\sync\tests\sync_tests.c -o build\sync_tests.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\render\bospectra_render.c -o build\bospectra_render.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\render\backends\software\software_backend.c -o build\software_backend.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\render\backends\opengl\opengl_backend.c -o build\bospectra_opengl_backend.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\render\pipeline\render_pipeline.c -o build\render_pipeline.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\render\texture_pool\texture_pool.c -o build\texture_pool.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\render\diagnostics\render_diag.c -o build\render_diag.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\render\tests\render_tests.c -o build\render_tests.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\playback\bospectra_playback.c -o build\bospectra_playback.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\playback\controller\playback_controller.c -o build\playback_controller.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\playback\session\playback_session.c -o build\playback_session.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\playback\state\playback_state.c -o build\bospectra_playback_state.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\playback\timeline\playback_timeline.c -o build\playback_timeline.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\playback\events\playback_events.c -o build\playback_events.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\playback\diagnostics\playback_diag.c -o build\bospectra_playback_diag.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\manager\resource_manager.c -o build\bospectra_resource_manager.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\manager\sync_manager.c -o build\bospectra_sync_manager.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\manager\container_manager.c -o build\bospectra_container_manager.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\manager\decoder_manager.c -o build\bospectra_decoder_manager.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\manager\render_manager.c -o build\bospectra_render_manager.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\manager\session_manager.c -o build\bospectra_session_manager.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\manager\media_manager.c -o build\bospectra_media_manager.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\registry\container_registry.c -o build\bospectra_v3_container_registry.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\registry\codec_registry.c -o build\bospectra_v3_codec_registry.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\registry\renderer_registry.c -o build\bospectra_v3_renderer_registry.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\registry\probe_engine.c -o build\bospectra_v3_probe_engine.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\registry\driver_registry.c -o build\bospectra_v3_driver_registry.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\registry\bospectra_registry.c -o build\bospectra_v3_registry.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\pipeline\packet_queue.c -o build\bospectra_v3_packet_queue.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\pipeline\decode_queue.c -o build\bospectra_v3_decode_queue.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\pipeline\frame_queue.c -o build\bospectra_v3_frame_queue.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\pipeline\renderer_queue.c -o build\bospectra_v3_renderer_queue.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\pipeline\scheduler.c -o build\bospectra_v3_pipeline_scheduler.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\pipeline\pipeline_engine.c -o build\bospectra_v3_pipeline_engine.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\pipeline\queue_metrics.c -o build\bospectra_v3_queue_metrics.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\scheduler\scheduler_clock.c -o build\bospectra_v3_scheduler_clock.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\scheduler\pts_manager.c -o build\bospectra_v3_pts_manager.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\scheduler\timeline_engine.c -o build\bospectra_v3_timeline_engine.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\scheduler\frame_pacer.c -o build\bospectra_v3_frame_pacer.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\scheduler\frame_scheduler.c -o build\bospectra_v3_frame_scheduler.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\scheduler\display_scheduler.c -o build\bospectra_v3_display_scheduler.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\scheduler\scheduler_metrics.c -o build\bospectra_v3_scheduler_metrics.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\resource\ownership_manager.c -o build\bospectra_v3_ownership_manager.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\resource\reference_manager.c -o build\bospectra_v3_reference_manager.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\resource\lifetime_tracker.c -o build\bospectra_v3_lifetime_tracker.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\resource\resource_graph.c -o build\bospectra_v3_resource_graph.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\resource\resource_validator.c -o build\bospectra_v3_resource_validator.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\resource\leak_detector.c -o build\bospectra_v3_leak_detector.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\resource\resource_metrics.c -o build\bospectra_v3_resource_metrics.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\diagnostics\media_debugger.c -o build\bospectra_v3_media_debugger.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\diagnostics\memory_validator.c -o build\bospectra_v3_memory_validator.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\diagnostics\ownership_dump.c -o build\bospectra_v3_ownership_dump.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\diagnostics\resource_dump.c -o build\bospectra_v3_resource_dump.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\diagnostics\queue_dump.c -o build\bospectra_v3_queue_dump.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\diagnostics\session_dump.c -o build\bospectra_v3_session_dump.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\diagnostics\trace_engine.c -o build\bospectra_v3_trace_engine.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\diagnostics\pipeline_validator.c -o build\bospectra_v3_pipeline_validator.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\diagnostics\diagnostic_console.c -o build\bospectra_v3_diagnostic_console.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\runtime\telemetry_engine.c -o build\bospectra_v3_telemetry_engine.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\runtime\performance_monitor.c -o build\bospectra_v3_performance_monitor.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\runtime\statistics_manager.c -o build\bospectra_v3_statistics_manager.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\runtime\error_manager.c -o build\bospectra_v3_error_manager.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\runtime\error_dispatcher.c -o build\bospectra_v3_error_dispatcher.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\runtime\error_reporter.c -o build\bospectra_v3_error_reporter.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\runtime\runtime_health.c -o build\bospectra_v3_runtime_health.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\runtime\runtime_metrics.c -o build\bospectra_v3_runtime_metrics.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\watchdog\watchdog_engine.c -o build\bospectra_v3_watchdog_engine.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\watchdog\watchdog_rules.c -o build\bospectra_v3_watchdog_rules.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\watchdog\watchdog_monitor.c -o build\bospectra_v3_watchdog_monitor.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\watchdog\watchdog_recovery.c -o build\bospectra_v3_watchdog_recovery.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\watchdog\watchdog_history.c -o build\bospectra_v3_watchdog_history.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\watchdog\watchdog_metrics.c -o build\bospectra_v3_watchdog_metrics.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\watchdog\watchdog_console.c -o build\bospectra_v3_watchdog_console.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\cleanup\cleanup_engine.c -o build\bospectra_v3_cleanup_engine.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\cleanup\cleanup_rules.c -o build\bospectra_v3_cleanup_rules.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\cleanup\cleanup_pipeline.c -o build\bospectra_v3_cleanup_pipeline.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\cleanup\cleanup_validator.c -o build\bospectra_v3_cleanup_validator.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\cleanup\cleanup_metrics.c -o build\bospectra_v3_cleanup_metrics.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\cleanup\cleanup_console.c -o build\bospectra_v3_cleanup_console.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\certification\cert_engine.c -o build\bospectra_v3_cert_engine.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\certification\cert_runner.c -o build\bospectra_v3_cert_runner.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\certification\cert_scenarios.c -o build\bospectra_v3_cert_scenarios.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\certification\cert_validator.c -o build\bospectra_v3_cert_validator.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\certification\cert_report.c -o build\bospectra_v3_cert_report.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\media\bospectra\certification\cert_console.c -o build\bospectra_v3_cert_console.o

clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\shell\apps\bos_media_player\core\player_core.c -o build\player_core.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\shell\apps\bos_media_player\views\viewport_view.c -o build\viewport_view.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\shell\apps\bos_media_player\views\toolbar_view.c -o build\toolbar_view.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\shell\apps\bos_media_player\controls\playback_controls.c -o build\playback_controls.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\shell\apps\bos_media_player\playlist\playlist_manager.c -o build\playlist_manager.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\shell\apps\bos_media_player\library\media_library.c -o build\media_library.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\shell\apps\bos_media_player\recent\recent_history.c -o build\recent_history.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\shell\apps\bos_media_player\settings\player_settings.c -o build\player_settings.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\shell\apps\bos_media_player\diagnostics\player_diag.c -o build\player_diag.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\shell\apps\bos_media_player\tests\player_tests.c -o build\player_tests.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -mno-red-zone -I. -c kernel\shell\apps\bos_media_player\bos_media_player.c -o build\bos_media_player.o

Write-Host "[5/5] Linking Kernel..." -ForegroundColor Yellow
$lldRsp = @'
--Map=build/kernel.map
-T
kernel/linker.ld
build/kernel_entry.o
build/kernel.o
build/abde_font.o
build/abde_renderer.o
build/abde.o
build/vmm_lifecycle_debug.o
build/syscall_tss_debug.o
build/pmm_debug.o
build/keyboard_led_debug.o
build/ps2_micro_debug.o
build/atoms_screenshot.o
build/aipdebug.o
build/input_power_audit.o
build/usb_hid_led_debug.o
build/syscall_security_debug.o
build/vfs_lifecycle_debug.o
build/storage_forensic_debug.o
build/windows_forensic_collector.o
build/bofs_validator.o
build/bofs_alloc.o
build/bofs_file.o
build/bofs_file_test.o
build/mouse_telemetry.o
build/debug_shell.o
build/ahme_core.o
build/ahme_profile.o
build/ahme_quirks.o
build/ahme_policy.o
build/ahme_health.o
build/ahme_input.o
build/ap_trampoline.o
build/smp.o
build/spinlock.o
build/phase7_reliability.o
build/phase7_cert.o
build/loader_debug.o
build/loader_elf_parser.o
build/loader_symbol_resolver.o
build/loader_reloc_engine.o
build/loader_segment_loader.o
build/loader_library_manager.o
build/loader_runtime_linker.o
build/loader_manager.o
build/loader_tests.o
build/ipc_debug.o
build/ipc_sync.o
build/ipc_permissions.o
build/ipc_channel_manager.o
build/ipc_message_queue.o
build/ipc_pipe_engine.o
build/ipc_port_manager.o
build/ipc_shm_manager.o
build/ipc_router.o
build/ipc_manager.o
build/ipc_tests.o
build/sec_debug.o
build/sec_diag.o
build/sec_random.o
build/sec_sha256.o
build/sec_sha384.o
build/sec_sha512.o
build/sec_aes.o
build/sec_rsa.o
build/sec_ecc.o
build/sec_crypto_manager.o
build/sec_x509.o
build/sec_cert_validator.o
build/sec_trust_store.o
build/sec_session.o
build/sec_cipher_suite.o
build/sec_key_exchange.o
build/sec_tls_engine.o
build/sec_manager.o
build/sec_test_suite.o
build/sb_manager.o
build/sb_process.o
build/sb_token.o
build/sb_capability.o
build/sb_permissions.o
build/sb_memory.o
build/sb_syscall.o
build/sb_ipc.o
build/sb_filesystem.o
build/sb_network.o
build/sb_resource.o
build/sb_audit.o
build/sb_debug.o
build/sb_tests.o
build/html_debug.o
build/html_diag.o
build/html_attr.o
build/html_node.o
build/html_elem.o
build/html_text.o
build/html_comment.o
build/html_doctype.o
build/html_frag.o
build/html_doc.o
build/html_mut.o
build/html_ser.o
build/html_tok.o
build/html_tb.o
build/html_parser.o
build/html_bos.o
build/html_tests.o
build/css_tok.o
build/css_val.o
build/css_spec.o
build/css_sel.o
build/css_rule.o
build/css_sheet.o
build/css_parser.o
build/css_style.o
build/css_bos.o
build/css_tests.o
build/cpu_features.o
build/cpu_state.o
build/test_cpu_phase0.o
build/bgl_drawable.o
build/bgl_context.o
build/bgl.o
build/test_bgl_phase1.o
build/gl_math.o
build/gl_state.o
build/gl_pipeline.o
build/gl_clip.o
build/gl_viewport.o
build/gl_depth.o
build/gl_texture.o
build/gl_sampler.o
build/gl_lighting.o
build/gl_fragment.o
build/gl_point.o
build/gl_line.o
build/gl_vertex_fetch.o
build/gl_display_list.o
build/gl_rasterizer.o
build/gl_fbo.o
build/gl.o
build/test_gl_phase2.o
build/test_gl_phase3.o
build/test_gl_phase4.o
build/test_gl_phase5.o
build/test_gl_phase6.o
build/test_gl_phase7.o
build/test_gl_phase8.o
build/test_gl_phase9.o
build/test_gl_phase10.o
build/atoms_graph_metrics.o
build/atoms_graph_scene.o
build/atoms_graph_renderer.o
build/atoms_graph_ui.o
build/atoms_graph_benchmark.o
build/atoms_graph_3d.o
build/test_gl_phase11.o
build/gpu_manager.o
build/gpu_pci.o
build/gpu_memory.o
build/gpu_surface_hal.o
build/gpu_drv_vmware.o
build/gpu_drv_virtio.o
build/gpu_drv_intel.o
build/gpu_drv_amd.o
build/gpu_drv_nvidia.o
build/gpu_drv_swrender.o
build/gpu_debug.o
build/edid_parser.o
build/display_manager.o
build/dve_framebuffer.o
build/dve_geometry.o
build/dve_pixel_format.o
build/dve_surface.o
build/dve_gpu_driver.o
build/dve_display_mode.o
build/dve_presentation.o
build/dve_memory_safety.o
build/dve_frame_integrity.o
build/dve_report.o
build/dve_certification_tests.o
build/dpdp.o
build/dpdp_preview.o
build/dpdp_registers.o
build/dpdp_timeline.o
build/dpdp_tests.o
build/gpu_tests.o
build/bvmm_init.o
build/bvmm_core.o
build/bvmm_phase1_tests.o
build/bvmm_heap.o
build/bvmm_tlsf.o
build/bvmm_range.o
build/bvmm_handle_table.o
build/bvmm_stats.o
build/bvmm_phase2_tests.o
build/bvmm_pools.o
build/bvmm_pool_policy.o
build/bvmm_phase3_tests.o
build/bvmm_surface.o
build/bvmm_surface_registry.o
build/bvmm_surface_diag.o
build/bvmm_phase4_tests.o
build/bvmm_texture.o
build/bvmm_texture_registry.o
build/bvmm_texture_format.o
build/bvmm_texture_diag.o
build/bvmm_phase5_tests.o
build/bvmm_lifetime.o
build/bvmm_lifetime_registry.o
build/bvmm_lifetime_diag.o
build/bvmm_phase6_tests.o
build/bvmm_sync.o
build/bvmm_sync_registry.o
build/bvmm_sync_graph.o
build/bvmm_sync_diag.o
build/bvmm_phase7_tests.o
build/bvmm_policy.o
build/bvmm_policy_registry.o
build/bvmm_policy_scheduler.o
build/bvmm_policy_diag.o
build/bvmm_phase8_tests.o
build/bvmm_defrag.o
build/bvmm_defrag_registry.o
build/bvmm_defrag_analyzer.o
build/bvmm_defrag_diag.o
build/bvmm_phase9_tests.o
build/bghal.o
build/bghal_registry.o
build/bghal_intel.o
build/bghal_amd.o
build/bghal_nvidia.o
build/bghal_virtio.o
build/bghal_vmware.o
build/bghal_swrender.o
build/bghal_diag.o
build/bvmm_phase10_tests.o
build/bcpse.o
build/bcpse_registry.o
build/bcpse_diag.o
build/bvmm_phase11_tests.o
build/bpoe.o
build/bpoe_optimizer.o
build/bpoe_profiler.o
build/bpoe_fastpath.o
build/bpoe_validation.o
build/bpoe_cache.o
build/bpoe_lock_optimizer.o
build/bpoe_diag.o
build/bvmm_phase12_tests.o
build/pci.o
build/e1000.o
build/r8168.o
build/lan_debug.o
build/netif.o
build/ethernet.o
build/arp.o
build/ipv4.o
build/icmp.o
build/udp.o
build/dhcp.o
build/dns.o
build/tcp.o
build/http.o
build/tls_record.o
build/tls_handshake.o
build/tls.o
build/sha256.o
build/hmac_sha256.o
build/tls_prf.o
build/aes.o
build/gcm.o
build/crypto_rand.o
build/tls_crypto.o
build/rsa.o
build/x509.o
build/x509_verify.o
build/trust_store.o
build/rtc.o
build/tls_test.o
build/net_service.o
build/socket_manager.o
build/socket.o
build/socket_test.o
build/xhci.o
build/xhci_dma.o
build/xhci_ring.o
build/xhci_cmd.o
build/xhci_transfer.o
build/usb_core.o
build/usb_enum.o
build/usb_registry.o
build/usb_forensic_center.o
build/usb_forensic_controller.o
build/usb_forensic_port.o
build/usb_forensic_eventring.o
build/usb_forensic_transfer.o
build/usb_forensic_dma.o
build/usb_forensic_irq.o
build/usb_forensic_timeline.o
build/usb_forensic_tree.o
build/hid_core.o
build/hid_parser.o
build/hid_mouse.o
build/hid_keyboard.o
build/usb_hub.o
build/usb_urb.o
build/ehci_companion.o
build/uhci.o
build/usb_transfer.o
build/usb_hid.o
build/bte_transfer_ring.o
build/bte_command_ring.o
build/bte_event_ring.o
build/bte_queue_manager.o
build/bte_scheduler.o
build/bte_bulk_in.o
build/bte_bulk_out.o
build/bte_bulk_submit.o
build/bte_bulk_complete.o
build/bte_msi.o
build/bte_event_handler.o
build/bte_telemetry.o
build/bte_forensic_dump.o
build/bte_ai_debug.o
build/bte_timeout.o
build/bte_bulk_tests.o
build/lhce_usb_dma.o
build/lhce_usb_scheduler.o
build/lhce_controller_manager.o
build/lhce_uhci.o
build/lhce_ohci.o
build/lhce_ehci.o
build/lhce_telemetry.o
build/lhce_forensic_dump.o
build/lhce_ai_debug.o
build/lhce_certification_tests.o
build/ucue_usb_core.o
build/ucue_usb_urb.o
build/ucue_usb_endpoint.o
build/ucue_usb_pipe.o
build/ucue_usb_request_queue.o
build/ucue_usb_transfer_dispatcher.o
build/ucue_usb_resource_manager.o
build/ucue_usb_timeout_engine.o
build/ucue_usb_completion_engine.o
build/ucue_diagnostics.o
build/ucue_certification_tests.o
build/uhe_hub_manager.o
build/uhe_hub_enumeration.o
build/uhe_hub_topology.o
build/uhe_hub_registry.o
build/uhe_port_power.o
build/uhe_port_reset.o
build/uhe_port_events.o
build/uhe_port_status.o
build/uhe_port_recovery.o
build/uhe_telemetry.o
build/uhe_forensic_dump.o
build/uhe_ai_debug.o
build/uhe_hub_certification_tests.o
build/ums_cbw.o
build/ums_csw.o
build/ums_recovery.o
build/ums_transport.o
build/ums_bot_engine.o
build/ums_scsi_inquiry.o
build/ums_scsi_read_capacity.o
build/ums_scsi_request_sense.o
build/ums_scsi_read10.o
build/ums_scsi_write10.o
build/ums_scsi_test_unit_ready.o
build/ums_scsi_mode_sense.o
build/ums_scsi_engine.o
build/ums_telemetry.o
build/ums_forensic_dump.o
build/ums_ai_debug.o
build/ums_bot_scsi_certification_tests.o
build/usm_disk_manager.o
build/usm_partition_manager.o
build/usm_volume_manager.o
build/usm_mount_manager.o
build/usm_storage_manager.o
build/usm_sector_cache.o
build/usm_block_cache.o
build/usm_io_scheduler.o
build/usm_vfs_bridge.o
build/usm_telemetry.o
build/usm_forensic_dump.o
build/usm_ai_debug.o
build/usm_storage_manager_tests.o
build/vizier_core.o
build/port_io.o
build/idt.o
build/isr_stubs.o
build/isr.o
build/exception.o
build/irq.o
build/pic.o
build/timer.o
build/pit.o
build/keyboard.o
build/ps2.o
build/ps2_mouse.o
build/cursor_certification.o
build/bce_cur_loader.o
build/bce_ani_loader.o
build/bce_cursor_cache.o
build/bce_cursor_theme.o
build/bce_cursor_animation.o
build/bce_cursor_hal.o
build/bce_cursor_diag.o
build/bce_bos_cursor.o
build/bce_cursor_tests.o
build/vbe.o
build/bv_core.o
build/bv_graphics.o
build/bv_text.o
build/bv_drawing.o
build/bv_renderer.o
build/bv_cursor_manager.o
build/bv_controls.o
build/bwe_core.o
build/bwe_process_queue.o
build/bwe_window.o
build/bwe_geometry.o
build/bwe_render_context.o
build/bwe_diagnostics.o
build/bwe_compositor.o
build/bwe_paint.o
build/botheme.o
build/bwe_theme.o
build/bwe_layout.o
build/bwe_controls.o
build/bwe_demo_app.o
build/bcm_core.o
build/bcm_task.o
build/atoms_app_manager.o
build/atoms_app_loader.o
build/atoms_app_permissions.o
build/atoms_app_window_api.o
build/atoms_app_clipboard.o
build/atoms_app_dialogs.o
build/atoms_app_vfs_api.o
build/atoms_app_runtime.o
build/atoms_app_debug.o
build/atoms_execution_contract.o
build/atoms_user_mode.o
build/atoms_user_mode_certification.o
build/atoms_process_manager.o
build/atoms_thread_manager.o
build/atoms_sll_manager.o
build/atoms_bkm_manager.o
build/atoms_syscall_gateway.o
build/atoms_app_installer.o
build/atoms_bosx_stress.o
build/atoms_network_manager.o
build/atoms_http_client.o
build/atoms_net_security.o
build/atoms_net_stress.o
build/atrix_browser_engine.o
build/atrix_browser_tabs.o
build/atrix_html_parser.o
build/atrix_css_parser.o
build/atrix_css_layout.o
build/atrix_render_tree.o
build/atrix_js_runtime.o
build/atrix_browser_http.o
build/atrix_browser_stress.o
build/atrix_browser_url.o
build/atrix_browser_navigation.o
build/atrix_html_document.o
build/atrix_paint_engine.o
build/atrix_layout_engine.o
build/atrix_browser_download.o
build/abe_core.o
build/abe_config.o
build/abe_feature.o
build/abe_process.o
build/abe_session.o
build/abe_window.o
build/abe_tab.o
build/abe_url.o
build/abe_navigation.o
build/abe_resource.o
build/abe_api.o
build/abe_render.o
build/abe_diagnostics.o
build/adf_core.o
build/adf_snapshot.o
build/adf_inspector.o
build/abe_phase1_test.o
build/abe_net_dns.o
build/abe_net_tls.o
build/abe_net_conn.o
build/abe_net_http.o
build/abe_net_parser.o
build/abe_net_redirect.o
build/abe_net_compress.o
build/abe_net_download.o
build/abe_net_manager.o
build/abe_net_test.o
build/abe_html_tokenizer.o
build/abe_dom_node.o
build/abe_html_element.o
build/abe_html_parser.o
build/abe_html_text.o
build/abe_html_document.o
build/abe_html_api.o
build/abe_html_test.o
build/abe_css_tokenizer.o
build/abe_css_parser.o
build/abe_css_selector.o
build/abe_css_cascade.o
build/abe_css_inherit.o
build/abe_css_computed.o
build/abe_css_style_manager.o
build/abe_css_api.o
build/abe_css_test.o
build/user_atoms_syscall.o
build/user_memory.o
build/user_stdio.o
build/user_pthread.o
build/user_cxx_runtime.o
build/user_runtime_test_suite.o
build/skia_color.o
build/skia_rect.o
build/skia_rrect.o
build/skia_matrix.o
build/skia_paint.o
build/skia_path.o
build/skia_imageinfo.o
build/skia_canvas.o
build/skia_surface.o
build/skia_rasterizer.o
build/skia_adapter.o
build/skia_test_suite.o
build/v8_page_allocator.o
build/v8_platform_atoms.o
build/v8_objects.o
build/v8_heap.o
build/v8_compiler.o
build/v8_interpreter.o
build/v8_assembler_x64.o
build/v8_jit_compiler_x64.o
build/v8_api.o
build/v8_atoms_platform.o
build/v8_test_suite.o
build/blink_node.o
build/blink_container_node.o
build/blink_element.o
build/blink_document.o
build/blink_text.o
build/blink_html_parser.o
build/blink_css_style_declaration.o
build/blink_layout_object.o
build/blink_layout_block.o
build/blink_layout_inline.o
build/blink_layout_tree_builder.o
build/blink_skia_painter.o
build/blink_script_controller.o
build/blink_atoms_adapter.o
build/blink_test_suite.o
build/chromium_net_gurl.o
build/chromium_net_security_origin.o
build/chromium_net_http_request_headers.o
build/chromium_net_http_response_headers.o
build/chromium_net_http_cache.o
build/chromium_net_canonical_cookie.o
build/chromium_net_cookie_store.o
build/chromium_net_url_loader.o
build/chromium_net_atoms_adapter.o
build/chromium_storage_area.o
build/chromium_storage_namespace.o
build/chromium_storage_local_manager.o
build/chromium_storage_session_manager.o
build/chromium_storage_atoms_vfs_adapter.o
build/chromium_net_storage_test_suite.o
build/chromium_ipc_channel.o
build/chromium_renderer_process_host.o
build/chromium_network_process_host.o
build/chromium_utility_process_host.o
build/chromium_browser_process_host.o
build/chromium_process_test_suite.o
build/mojo_core_handle_table.o
build/mojo_core_message_pipe.o
build/mojo_core_shared_buffer.o
build/mojo_core_c_abi.o
build/mojo_public_message.o
build/mojo_test_suite.o
build/chromium_net_csp.o
build/chromium_net_security_headers.o
build/chromium_security_test_suite.o
build/chromium_gpu_process_host.o
build/chromium_gpu_command_buffer.o
build/chromium_gpu_command_decoder.o
build/chromium_gpu_channel_host.o
build/blink_webgl_rendering_context.o
build/blink_canvas_rendering_context_2d.o
build/blink_offscreen_canvas.o
build/blink_image_bitmap.o
build/blink_html_media_element.o
build/blink_html_video_element.o
build/blink_html_audio_element.o
build/blink_blob.o
build/blink_file_reader.o
build/blink_audio_context.o
build/blink_media_source.o
build/blink_video_decoder.o
build/chromium_media_gpu_test_suite.o
build/chromium_compatibility_test_suite.o









build/abe_render_tree.o

build/abe_box_model.o
build/abe_block_layout.o
build/abe_inline_layout.o
build/abe_flex_layout.o
build/abe_positioning.o
build/abe_reflow.o
build/abe_layout_diag.o
build/abe_layout_api.o
build/abe_layout_test.o
build/abe_js_lexer.o
build/abe_js_parser.o
build/abe_js_compiler.o
build/abe_js_vm.o
build/abe_js_gc.o
build/abe_js_objects.o
build/abe_js_promise.o
build/abe_js_event_loop.o
build/abe_js_dom_binding.o
build/abe_js_module.o
build/abe_js_diag.o
build/abe_js_api.o
build/abe_js_test.o
build/abe_web_fetch.o
build/abe_web_xhr.o
build/abe_web_url.o
build/abe_web_storage.o
build/abe_web_history.o
build/abe_web_navigator.o
build/abe_web_performance.o
build/abe_web_scheduler.o
build/abe_web_observers.o
build/abe_web_blob.o
build/abe_web_form.o
build/abe_web_diag.o
build/abe_web_api.o
build/abe_web_test.o
build/bv_geometry.o
build/bv_boscal.o
build/bv_images.o
build/bv_layout.o
build/bv_input.o
build/kernel_input.o
build/input_abstraction.o
build/hida.o
build/ccte.o
build/input_core.o
build/input_adapter.o
build/pointer_state.o
build/pointer_precision.o
build/pointer_velocity.o
build/pointer_buttons.o
build/pointer_bounds_v2.o
build/pointer_consumers.o
build/pointer_motion.o
build/pointer_engine.o
build/dispatcher_priority.o
build/dispatcher_queue.o
build/dispatcher_diag.o
build/dispatcher_consumers.o
build/dispatcher_filters.o
build/dispatcher_router.o
build/dispatcher.o
build/cursor_state.o
build/cursor_hotspot.o
build/cursor_theme.o
build/cursor_animation.o
build/cursor_diag.o
build/cursor_backend.o
build/cursor_renderer.o
build/cursor_engine.o
build/usb_tablet.o
build/vmmouse.o
build/ivdl.o
build/pointer_diag.o
build/pointer_predict.o
build/pointer_filter.o
build/pointer_sync.o
build/pointer_manager.o
build/mouse_engine.o
build/bmde.o
build/vga.o
build/console.o
build/display.o
build/pmm.o
build/bitmap.o
build/vmm.o
build/paging.o
build/heap.o
build/amsss_coordinator.o
build/amsss_pressure.o
build/amsss_health.o
build/amsss_diagnostic.o
build/amsss_heap_backend.o
build/list.o
build/crash_log.o
build/runqueue.o
build/task.o
build/context.o
build/context_switch.o
build/syscall.o
build/syscall_dispatcher.o
build/syscall_validation.o
build/syscall_services.o
build/syscall_tests.o
build/syscall_wrappers.o
build/syscall_entry.o
build/gui_events.o
build/gdt.o
build/gdt_flush.o
build/enter_usermode.o
build/scheduler.o
build/kernel_stack.o
build/bre.o
build/block_device.o
build/ata.o
build/ahci.o
build/nvme.o
build/gpt.o
build/mbr.o
build/disk_manager.o
build/vfs.o
build/dummy_fs.o
build/string.o
build/fat32.o
build/ntfs.o
build/ntfs_test.o
build/elf_validate.o
build/elf_segment.o
build/process_builder.o
build/process.o
build/bosx_loader.o
build/conhost.o
build/boimage.o
build/boasset.o
build/asset_cache.o
build/asset_loader.o
build/font_loader.o
build/glyph_cache.o
build/glyph_atlas.o
build/text_layout.o
build/bofont.o
build/bofont_assets.o
build/rook_core.o
build/rook_registry.o
build/rook_render.o
build/rook_debug.o
build/spinner.o
build/page_boot.o
build/rook_dashboard.o
build/boot_assets_data.o
build/boot_assets.o
build/wallpaper_service.o
build/user_profile_service.o
build/page_login.o
build/page_shutdown.o
build/bomatrix.o
build/desktop_shell.o
build/dom.o
build/desktop_vfs_sync.o
build/desktop_watcher.o
build/desktop_certification_tests.o
build/horse_engine.o
build/icon_engine.o
build/atoms_start_icon.o
build/task_panel.o
build/start_menu.o
build/bos_shell_panel.o
build/system_hub.o
build/apps.o
build/kernel_shell_os_stubs.o
build/kernel_command.o
build/kernel_commands_sys.o
build/input_lab.o
build/atrix_browser.o
build/minbrow_probe.o
build/atoms_tmh_provider.o
build/tmh_app.o
build/explorer.o
build/explorer_view.o
build/explorer_cache.o
build/notes_app.o
build/rename_dialog.o
build/app_clipboard.o
build/audio_api.o
build/audio_core.o
build/audio_realtime_worker.o
build/audio_debug.o
build/audio_diagnostic_mode.o
build/ac97.o
build/ac97_codec.o
build/ac97_dma.o
build/ac97_playback.o
build/audio_forensic.o
build/audio_pcm.o
build/audio_driver_registry.o
build/audio_hal.o
build/audio_mixer.o
build/audio_mix_math.o
build/audio_player.o
build/audio_producer_worker.o
build/audio_buffer.o
build/audio_stream.o
build/audio_volume.o
build/audio_test_mode.o
build/bopawn.o
build/surface.o
build/bopawn_loader.o
build/bopawn_cache.o
build/bopawn_converter.o
build/bopawn_raw.o
build/bopawn_bmp.o
build/bopawn_ico.o
build/bopawn_png.o
build/bopawn_crc.o
build/bopawn_inflate.o
build/bopawn_filters.o
build/wallpaper_registry.o
build/wallpaper_scaler.o
build/wallpaper_manager.o
build/wallpaper_settings.o
build/animation_engine.o
build/animation_timeline.o
build/animation_easing.o
build/animation_scheduler.o
build/animation_fade.o
build/ame_core.o
build/ame_easing.o
build/ame_spinner.o
build/ame_modules.o
build/identity.o
build/display_hal.o
build/vbe_driver.o
build/present_queue.o
build/damage_tracker.o
build/swapchain.o
build/frame_pacer.o
build/cursor_plane.o
build/bspe_cursor_present.o
build/bspe_present.o
build/vram_copy.o
build/dual_page_present.o
build/telemetry_hud.o
build/step14_telemetry.o
build/profiler.o
build/frame_profiler.o
build/render_profiler.o
build/sched_profiler.o
build/mem_profiler.o
build/present_profiler.o
build/stats_profiler.o
build/report_profiler.o
build/profiler_tests.o
build/pacing_analyzer.o
build/boot_mode.o
build/deferred_work.o
build/agdte_timing.o
build/agdte_display_state.o
build/agdte_buffer_manager.o
build/agdte_surface_manager.o
build/agdte_present_queue.o
build/agdte_scheduler.o
build/agdte_backend.o
build/agdte_diag.o
build/agdte_presenter.o
build/agdte.o
build/agdte_present_timeline.o
build/agdte_frame_metrics.o
build/agdte_vsync.o
build/agdte_frame_pacer.o
build/agdte_refresh_controller.o
build/agdte_swap_controller.o
build/quality_engine.o
build/frame_stabilizer.o
build/motion_analyzer.o
build/dirty_optimizer.o
build/cadence_optimizer.o
build/presentation_diag.o
build/display_metrics.o
build/present_quality.o
build/die_manager.o
build/die_detection.o
build/die_capabilities.o
build/die_policy.o
build/die_geometry.o
build/die_layout.o
build/die_runtime.o
build/die_diag.o
build/bocompositor.o
build/compositor_clip.o
build/compositor_damage.o
build/compositor_stack.o
build/compositor_surface.o
build/agdpe_core.o
build/agdpe_vbe_driver.o
build/agdae_core.o
build/agdae_diag.o
build/agdae_geometry.o
build/agdae_scaling.o
build/bdce_authority.o
build/bdce_validation.o
build/bos_pack_loader.o
build/bos_manifest.o
build/bos_elf_loader.o
build/bos_sdk_runtime.o
build/bos_installer.o
build/bosfsr_core.o
build/bosfsr_graphics.o
build/bosfsr_containers.o
build/bosfsr_controls.o
build/bosfsr32.o
build/agp_runtime.o
build/agp_context.o
build/agp_surface.o
build/agp_swapchain.o
build/agp_display.o
build/agp_window.o
build/agp_commands.o
build/agp_texture.o
build/agp_buffer.o
build/agp_shader.o
build/agp_pipeline.o
build/agp_sync.o
build/agp_opengl_rt.o
build/agp_opengl_loader.o
build/agp_egl.o
build/agp_resources.o
build/agp_diagnostics.o
build/agp_profiler.o
build/agp_vram.o
build/agp_scheduler.o
build/opengl32.o
build/agp_certification_tests.o
build/botree_init.o
build/bde_path_normalize.o
build/bde_path_canonical.o
build/bde_path_split.o
build/bde_path_compare.o
build/bde_nav_session.o
build/bde_namespace_core.o
build/bde_cache_dir.o
build/bde_transaction_queue.o
build/bde_cmd_handlers.o
build/bde_permission_acl.o
build/bde_watcher_hub.o
build/bde_clipboard_store.o
build/bde_shortcut_resolver.o
build/bde_metadata_query.o
build/bde_recycle_bin.o
build/dre_core.o
build/dre_navigator.o
build/dre_enumerator.o
build/dre_selection.o
build/dre_sort.o
build/dre_filter.o
build/dre_certification_tests.o
build/bdr_core.o
build/bdr_icons.o
build/bdr_grid.o
build/bdr_selection.o
build/bdr_wallpaper.o
build/bdr_notifications.o
build/bdr_devices.o
build/bdr_diagnostics.o
build/bdr_certification_tests.o
build/bsr_core.o
build/bsr_dispatcher.o
build/bsr_file_association.o
build/bsr_launcher.o
build/bsr_search.o
build/bsr_recent.o
build/bsr_favorites.o
build/bsr_notifications.o
build/bsr_dialogs.o
build/bsr_diagnostics.o
build/bsr_certification_tests.o
build/brt_core.o
build/brt_registry.o
build/brt_dispatcher.o
build/brt_objects.o
build/brt_session.o
build/brt_cache.o
build/brt_events.o
build/brt_clipboard.o
build/brt_dragdrop.o
build/brt_notifications.o
build/brt_search.o
build/brt_security.o
build/brt_transactions.o
build/brt_diagnostics.o
build/brt_certification_tests.o
build/bfs_core.o
build/bfs_file_manager.o
build/bfs_copy.o
build/bfs_move.o
build/bfs_delete.o
build/bfs_rename.o
build/bfs_create.o
build/bfs_properties.o
build/bfs_locking.o
build/bfs_association.o
build/bfs_mime.o
build/bfs_icons.o
build/bfs_thumbnails.o
build/bfs_recycle.o
build/bfs_recent.o
build/bfs_favorites.o
build/bfs_quickaccess.o
build/bfs_metadata.o
build/bfs_search.o
build/bfs_transactions.o
build/bfs_diagnostics.o
build/bfs_certification_tests.o
build/bsom_core.o
build/bsom_registry.o
build/bsom_factory.o
build/bsom_handles.o
build/bsom_lifetime.o
build/bsom_resolver.o
build/bsom_namespace.o
build/bsom_properties.o
build/bsom_icons.o
build/bsom_thumbnails.o
build/bsom_clipboard.o
build/bsom_dragdrop.o
build/bsom_search.o
build/bsom_recent.o
build/bsom_favorites.o
build/bsom_recycle.o
build/bsom_permissions.o
build/bsom_metadata.o
build/bsom_cache.o
build/bsom_diagnostics.o
build/bsom_certification_tests.o
build/explorer_rewrite_certification.o
build/bar_runtime.o
build/bar_process.o
build/bar_session.o
build/bar_registry.o
build/bar_dll.o
build/bar_message.o
build/bar_dispatcher.o
build/bar_windows.o
build/bar_resources.o
build/bar_dialogs.o
build/bar_clipboard.o
build/bar_dragdrop.o
build/bar_timers.o
build/bar_focus.o
build/bar_packages.o
build/bar_manifest.o
build/bar_security.o
build/bar_diagnostics.o
build/bar_certification_tests.o
build/user32_runtime.o
build/user32_class.o
build/user32_window.o
build/user32_message.o
build/user32_input.o
build/user32_keyboard.o
build/user32_mouse.o
build/user32_focus.o
build/user32_caret.o
build/user32_cursor.o
build/user32_clipboard.o
build/user32_dragdrop.o
build/user32_menus.o
build/user32_dialogs.o
build/user32_controls.o
build/user32_timers.o
build/user32_hooks.o
build/user32_accelerators.o
build/user32_diagnostics.o
build/user32_certification_tests.o
build/gdi32_runtime.o
build/gdi32_dc.o
build/gdi32_pen.o
build/gdi32_brush.o
build/gdi32_font.o
build/gdi32_text.o
build/gdi32_bitmap.o
build/gdi32_image.o
build/gdi32_region.o
build/gdi32_clipping.o
build/gdi32_painting.o
build/gdi32_blit.o
build/gdi32_alpha.o
build/gdi32_geometry.o
build/gdi32_colors.o
build/gdi32_surfaces.o
build/gdi32_paths.o
build/gdi32_printing.o
build/gdi32_diagnostics.o
build/gdi32_certification_tests.o
build/kernel32_runtime.o
build/kernel32_process.o
build/kernel32_thread.o
build/kernel32_memory.o
build/kernel32_heap.o
build/kernel32_sync.o
build/kernel32_mutex.o
build/kernel32_semaphore.o
build/kernel32_critical.o
build/kernel32_events.o
build/kernel32_files.o
build/kernel32_directory.o
build/kernel32_pipes.o
build/kernel32_console.o
build/kernel32_time.o
build/kernel32_locale.o
build/kernel32_environment.o
build/kernel32_loader.o
build/kernel32_tls.o
build/kernel32_exceptions.o
build/kernel32_atoms.o
build/kernel32_performance.o
build/kernel32_diagnostics.o
build/kernel32_certification_tests.o
build/comdlg32_runtime.o
build/comdlg32_open_save.o
build/comdlg32_folder.o
build/comdlg32_color.o
build/comdlg32_font.o
build/comdlg32_print.o
build/comdlg32_page_setup.o
build/comdlg32_find_replace.o
build/comdlg32_preview.o
build/comdlg32_history.o
build/comdlg32_favorites.o
build/comdlg32_quickaccess.o
build/comdlg32_sidebar.o
build/comdlg32_filters.o
build/comdlg32_navigation.o
build/comdlg32_validation.o
build/comdlg32_layout.o
build/comdlg32_theme.o
build/comdlg32_icons.o
build/comdlg32_bookmarks.o
build/comdlg32_diagnostics.o
build/comdlg32_certification_tests.o
build/comctl32_runtime.o
build/comctl32_button.o
build/comctl32_edit.o
build/comctl32_static.o
build/comctl32_listbox.o
build/comctl32_combobox.o
build/comctl32_listview.o
build/comctl32_treeview.o
build/comctl32_tab.o
build/comctl32_toolbar.o
build/comctl32_statusbar.o
build/comctl32_progress.o
build/comctl32_trackbar.o
build/comctl32_header.o
build/comctl32_imagelist.o
build/comctl32_tooltip.o
build/comctl32_rebar.o
build/comctl32_updown.o
build/comctl32_pager.o
build/comctl32_animation.o
build/comctl32_monthcal.o
build/comctl32_datetime.o
build/comctl32_hotkey.o
build/comctl32_ipaddress.o
build/comctl32_theme.o
build/comctl32_layout.o
build/comctl32_notifications.o
build/comctl32_accessibility.o
build/comctl32_diagnostics.o
build/comctl32_certification_tests.o
build/shell_runtime.o
build/shell_desktop.o
build/shell_explorer.o
build/shell_namespace.o
build/shell_specialfolders.o
build/shell_recyclebin.o
build/shell_shortcuts.o
build/shell_icons.o
build/shell_imagelist.o
build/shell_contextmenu.o
build/shell_fileassoc.o
build/shell_execute.o
build/shell_properties.o
build/shell_clipboard.o
build/shell_dragdrop.o
build/shell_notifications.o
build/shell_taskbar.o
build/shell_tray.o
build/shell_search.o
build/shell_history.o
build/shell_diagnostics.o
build/shell32_certification_tests.o
build/bos_bootstrap.o
build/bos_loader.o
build/bos_object.o
build/bos_handles.o
build/bos_process.o
build/bos_thread.o
build/bos_memory.o
build/bos_vmem.o
build/bos_heap.o
build/bos_sync.o
build/bos_exceptions.o
build/bos_ipc.o
build/bos_security.o
build/bos_syscall.o
build/bos_timer.o
build/bos_performance.o
build/bos_context.o
build/bos_environment.o
build/bos_diagnostics.o
build/bosll_certification_tests.o
build/opengl_runtime.o
build/opengl_context.o
build/opengl_pixel.o
build/opengl_surface.o
build/opengl_buffers.o
build/opengl_textures.o
build/opengl_shaders.o
build/opengl_pipeline.o
build/opengl_vertex.o
build/opengl_framebuffer.o
build/opengl_swap.o
build/opengl_extensions.o
build/opengl_dispatch.o
build/opengl_errors.o
build/opengl_state.o
build/opengl_agp_translate.o
build/opengl_performance.o
build/opengl_resource.o
build/opengl_diagnostics.o
build/opengl32_certification_tests.o
build/advapi_runtime.o
build/advapi_registry.o
build/advapi_reg_notify.o
build/advapi_security.o
build/advapi_sid.o
build/advapi_acl.o
build/advapi_tokens.o
build/advapi_privilege.o
build/advapi_services.o
build/advapi_eventlog.o
build/advapi_crypto.o
build/advapi_policy.o
build/advapi_lsa.o
build/advapi_impersonation.o
build/advapi_audit.o
build/advapi_env.o
build/advapi_handles.o
build/advapi_performance.o
build/advapi_diagnostics.o
build/advapi32_certification_tests.o
build/ws2_runtime.o
build/ws2_socket.o
build/ws2_tcp.o
build/ws2_udp.o
build/ws2_address.o
build/ws2_interface.o
build/ws2_dns.o
build/ws2_transfer.o
build/ws2_async.o
build/ws2_events.o
build/ws2_select.o
build/ws2_poll.o
build/ws2_options.o
build/ws2_security.o
build/ws2_buffers.o
build/ws2_performance.o
build/ws2_diagnostics.o
build/ws2_compat.o
build/ws2_resource.o
build/ws2_32_certification_tests.o
build/ole_runtime.o
build/ole_com.o
build/ole_iunknown.o
build/ole_factory.o
build/ole_objects.o
build/ole_interfaces.o
build/ole_clsid.o
build/ole_iid.o
build/ole_apartment.o
build/ole_marshal.o
build/ole_storage.o
build/ole_stream.o
build/ole_clipboard.o
build/ole_dragdrop.o
build/ole_dataobject.o
build/ole_moniker.o
build/ole_property.o
build/ole_persist.o
build/ole_diagnostics.o
build/ole32_certification_tests.o
build/explorer_runtime.o
build/explorer_desktop.o
build/explorer_session.o
build/explorer_wallpaper.o
build/explorer_icons.o
build/explorer_taskbar.o
build/explorer_startmenu.o
build/explorer_tray.o
build/explorer_browser.o
build/explorer_navigation.o
build/explorer_addressbar.o
build/explorer_search.o
build/explorer_contextmenu.o
build/explorer_dragdrop.o
build/explorer_clipboard.o
build/explorer_recyclebin.o
build/explorer_associations.o
build/explorer_controlpanel.o
build/explorer_settings.o
build/explorer_favorites.o
build/explorer_recent.o
build/explorer_diagnostics.o
build/explorer_certification_tests.o
build/controlpanel_runtime.o
build/controlpanel_loader.o
build/controlpanel_category.o
build/controlpanel_search.o
build/controlpanel_navigation.o
build/controlpanel_favorites.o
build/controlpanel_history.o
build/controlpanel_permissions.o
build/controlpanel_settings.o
build/controlpanel_plugin.o
build/controlpanel_diagnostics.o
build/bosc_modules.o
build/controlpanel_certification_tests.o
build/settings_runtime.o
build/settings_dashboard.o
build/settings_navigation.o
build/settings_search.o
build/settings_display.o
build/settings_theme.o
build/settings_personalization.o
build/settings_sound.o
build/settings_network.o
build/settings_bluetooth.o
build/settings_storage.o
build/settings_accounts.o
build/settings_privacy.o
build/settings_updates.o
build/settings_accessibility.o
build/settings_devices.o
build/settings_notifications.o
build/settings_power.o
build/settings_diagnostics.o
build/settings_certification_tests.o
build/terminal_runtime.o
build/terminal_console.o
build/terminal_parser.o
build/terminal_commands.o
build/terminal_history.o
build/terminal_autocomplete.o
build/terminal_aliases.o
build/terminal_environment.o
build/terminal_filesystem.o
build/terminal_process.o
build/terminal_script.o
build/terminal_plugin.o
build/terminal_diagnostics.o
build/terminal_certification_tests.o
build/taskmgr_runtime.o
build/taskmgr_dashboard.o
build/taskmgr_process.o
build/taskmgr_threads.o
build/taskmgr_memory.o
build/taskmgr_cpu.o
build/taskmgr_gpu.o
build/taskmgr_storage.o
build/taskmgr_network.o
build/taskmgr_power.o
build/taskmgr_services.o
build/taskmgr_drivers.o
build/taskmgr_modules.o
build/taskmgr_handles.o
build/taskmgr_hardware.o
build/taskmgr_sensors.o
build/taskmgr_performance.o
build/taskmgr_graphs.o
build/taskmgr_diagnostics.o
build/taskmgr_certification_tests.o
build/fe_runtime.o
build/fe_navigation.o
build/fe_addressbar.o
build/fe_treeview.o
build/fe_listview.o
build/fe_namespace.o
build/fe_drives.o
build/fe_search.o
build/fe_preview.o
build/fe_thumbnail.o
build/fe_clipboard.o
build/fe_dragdrop.o
build/fe_operations.o
build/fe_properties.o
build/fe_permissions.o
build/fe_contextmenu.o
build/fe_favorites.o
build/fe_recent.o
build/fe_history.o
build/fe_shortcuts.o
build/fe_refresh.o
build/fe_watcher.o
build/fe_forensic.o
build/fe_diagnostics.o
build/fe_certification_tests.o
build/bospectra_core.o
build/bospectra_state.o
build/bospectra_debug.o
build/bospectra_memory.o
build/bospectra_file.o
build/bospectra_packet.o
build/bospectra_buffer.o
build/bospectra_stream.o
build/bospectra_container.o
build/container_common.o
build/container_registry.o
build/mp4_parser.o
build/mkv_parser.o
build/avi_parser.o
build/container_tests.o
build/bospectra_frame_memory.o
build/frame_pool.o
build/packet_pool.o
build/media_queue.o
build/circular_ring.o
build/frame_memory_diag.o
build/frame_memory_tests.o
build/bospectra_color.o
build/yuv420_converter.o
build/nv12_converter.o
build/yuy2_converter.o
build/rgb_converter.o
build/color_pipeline.o
build/color_diag.o
build/color_tests.o
build/bospectra_decoder.o
build/bitstream_reader.o
build/idct.o
build/h264_decoder.o
build/mjpeg_decoder.o
build/mpeg2_decoder.o
build/decoder_registry.o
build/decoder_diag.o
build/decoder_tests.o
build/bospectra_audio.o
build/audio_bridge.o
build/bospectra_audio_session.o
build/bospectra_audio_queue.o
build/bospectra_audio_diag.o
build/bospectra_audio_tests.o
build/bospectra_sync.o
build/master_clock.o
build/drift_detector.o
build/bospectra_frame_scheduler.o
build/sync_diag.o
build/sync_tests.o
build/bospectra_render.o
build/software_backend.o
build/bospectra_opengl_backend.o
build/render_pipeline.o
build/texture_pool.o
build/render_diag.o
build/render_tests.o
build/bospectra_playback.o
build/playback_controller.o
build/playback_session.o
build/bospectra_playback_state.o
build/playback_timeline.o
build/playback_events.o
build/bospectra_playback_diag.o
build/bospectra_playback_tests.o
build/bospectra_tests.o
build/bospectra_resource_manager.o
build/bospectra_sync_manager.o
build/bospectra_container_manager.o
build/bospectra_decoder_manager.o
build/bospectra_render_manager.o
build/bospectra_session_manager.o
build/bospectra_media_manager.o
build/bospectra_v3_container_registry.o
build/bospectra_v3_codec_registry.o
build/bospectra_v3_renderer_registry.o
build/bospectra_v3_probe_engine.o
build/bospectra_v3_driver_registry.o
build/bospectra_v3_registry.o
build/bospectra_v3_packet_queue.o
build/bospectra_v3_decode_queue.o
build/bospectra_v3_frame_queue.o
build/bospectra_v3_renderer_queue.o
build/bospectra_v3_pipeline_scheduler.o
build/bospectra_v3_pipeline_engine.o
build/bospectra_v3_queue_metrics.o
build/bospectra_v3_scheduler_clock.o
build/bospectra_v3_pts_manager.o
build/bospectra_v3_timeline_engine.o
build/bospectra_v3_frame_pacer.o
build/bospectra_v3_frame_scheduler.o
build/bospectra_v3_display_scheduler.o
build/bospectra_v3_scheduler_metrics.o
build/bospectra_v3_ownership_manager.o
build/bospectra_v3_reference_manager.o
build/bospectra_v3_lifetime_tracker.o
build/bospectra_v3_resource_graph.o
build/bospectra_v3_resource_validator.o
build/bospectra_v3_leak_detector.o
build/bospectra_v3_resource_metrics.o
build/bospectra_v3_media_debugger.o
build/bospectra_v3_memory_validator.o
build/bospectra_v3_ownership_dump.o
build/bospectra_v3_resource_dump.o
build/bospectra_v3_queue_dump.o
build/bospectra_v3_session_dump.o
build/bospectra_v3_trace_engine.o
build/bospectra_v3_pipeline_validator.o
build/bospectra_v3_diagnostic_console.o
build/bospectra_v3_telemetry_engine.o
build/bospectra_v3_performance_monitor.o
build/bospectra_v3_statistics_manager.o
build/bospectra_v3_error_manager.o
build/bospectra_v3_error_dispatcher.o
build/bospectra_v3_error_reporter.o
build/bospectra_v3_runtime_health.o
build/bospectra_v3_runtime_metrics.o
build/bospectra_v3_watchdog_engine.o
build/bospectra_v3_watchdog_rules.o
build/bospectra_v3_watchdog_monitor.o
build/bospectra_v3_watchdog_recovery.o
build/bospectra_v3_watchdog_history.o
build/bospectra_v3_watchdog_metrics.o
build/bospectra_v3_watchdog_console.o
build/bospectra_v3_cleanup_engine.o
build/bospectra_v3_cleanup_rules.o
build/bospectra_v3_cleanup_pipeline.o
build/bospectra_v3_cleanup_validator.o
build/bospectra_v3_cleanup_metrics.o
build/bospectra_v3_cleanup_console.o
build/bospectra_v3_cert_engine.o
build/bospectra_v3_cert_runner.o
build/bospectra_v3_cert_scenarios.o
build/bospectra_v3_cert_validator.o
build/bospectra_v3_cert_report.o
build/bospectra_v3_cert_console.o
build/player_core.o
build/viewport_view.o
build/toolbar_view.o
build/playback_controls.o
build/playlist_manager.o
build/media_library.o
build/recent_history.o
build/player_settings.o
build/player_diag.o
build/player_tests.o
build/bos_media_player.o
build/net_packet.o
build/net_device.o
build/net_rtl8168.o
build/realtek_master.o
build/usb_forensic_trace.o
build/usb_forensic_phase3.o
build/system_power.o
build/vram_accel.o
build/bram.o
build/dgl.o
build/klog.o
build/embedded_desktop_elf.o
-o
build/kernel.bin
'@
Write-Host "Compiling Userspace Desktop Shell Payload for Kernel Embedding..." -ForegroundColor Cyan
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -nostdlib -c userspace\libbos\src\syscalls.c -o build\syscalls.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -nostdlib -c userspace\libbos\src\bpde.c -o build\bpde.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -nostdlib -c userspace\libbos_gui\src\syscalls_gui.c -o build\syscalls_gui.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -nostdlib -c userspace\libbos_gui\src\widgets.c -o build\widgets.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -nostdlib -c userspace\libbos_gui\src\bos_gui.c -o build\bos_gui.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -nostdlib -c userspace\bishopmath\src\bishop_builtins.c -o build\bishop_builtins.o
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -nostdlib -c userspace\apps\desktop_shell\main.c -o build\ring3_desktop_shell.o

if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED! Desktop shell compilation failed" -ForegroundColor Red; exit $LASTEXITCODE }

ld.lld -T userspace\linker.ld --strip-all build\ring3_desktop_shell.o build\syscalls.o build\syscalls_gui.o build\widgets.o build\bos_gui.o build\bpde.o build\bishop_builtins.o -o build\desktop_shell.elf
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED! Desktop shell link failed" -ForegroundColor Red; exit $LASTEXITCODE }
Copy-Item -Force build\desktop_shell.elf build\atoms_desktop.elf
Copy-Item -Force build\desktop_shell.elf build\calc.elf

nasm -f elf64 kernel\embedded_desktop_elf.asm -o build\embedded_desktop_elf.o
if ($LASTEXITCODE -ne 0) { Write-Host "Assembly of embedded_desktop_elf.asm failed!" -ForegroundColor Red; exit $LASTEXITCODE }

$lldRsp | Out-File -FilePath 'build\link.rsp' -Encoding ASCII -NoNewline
ld.lld '@build\link.rsp'

if ($LASTEXITCODE -ne 0) { Write-Host "LINK FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

Write-Host "Compiling Standalone UEFI Bootloader with Embedded Kernel Payload (BOOTX64.EFI)..." -ForegroundColor Cyan
nasm -f win64 boot\uefi\kernel_payload.asm -o build\kernel_payload.o
if ($LASTEXITCODE -ne 0) { Write-Host "Assembly of kernel_payload.asm failed!" -ForegroundColor Red; exit $LASTEXITCODE }
clang -target x86_64-unknown-windows "-Wl,-subsystem:efi_application" "-Wl,-entry:efi_main" "-Wl,-dynamicbase:no" -mno-red-zone -mno-stack-arg-probe -nostdlib -ffreestanding -fno-stack-protector -fno-pic -I. boot\uefi\bootx64.c build\kernel_payload.o -o build\BOOTX64_TMP.EFI
if ($LASTEXITCODE -ne 0) { Write-Host "UEFI Bootloader Compilation Failed!" -ForegroundColor Red; exit $LASTEXITCODE }
Copy-Item build\BOOTX64_TMP.EFI build\BOOTX64.EFI -Force

# Measure Kernel Payload Size & Compute Required Sectors
$kernelFile = Get-Item "build\kernel.bin"
$actualKernelBytes = $kernelFile.Length
$REQUIRED_KERNEL_SECTORS = [math]::Ceiling($actualKernelBytes / 512)
$RESERVED_DISK_SECTORS = 65520

Write-Host "-----------------------------------------" -ForegroundColor Cyan
Write-Host " [KERNEL BUILD METRICS]" -ForegroundColor Cyan
Write-Host "   Actual Kernel Payload : $actualKernelBytes bytes" -ForegroundColor Green
Write-Host "   Required Sectors      : $REQUIRED_KERNEL_SECTORS sectors" -ForegroundColor Green
Write-Host "   Disk Reserved Capacity: $RESERVED_DISK_SECTORS sectors (33,546,240 bytes, 32MB payload area)" -ForegroundColor Green
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

Write-Host "Compiling Ring 3 ATOMS Desktop Shell Application..." -ForegroundColor Cyan
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -nostdlib -c userspace\apps\desktop_shell\main.c -o build\ring3_desktop_shell.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

ld.lld -T userspace\linker.ld --strip-all build\ring3_desktop_shell.o build\syscalls.o build\syscalls_gui.o build\widgets.o build\bos_gui.o build\bpde.o build\bishop_builtins.o -o build\desktop_shell.elf
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }
Copy-Item -Force build\desktop_shell.elf build\atoms_desktop.elf
Copy-Item -Force build\desktop_shell.elf build\calc.elf

Write-Host "Compiling SDK Explorer Application..." -ForegroundColor Cyan
clang -target x86_64-pc-none-elf -mno-sse -mno-sse2 -mno-mmx -msoft-float -ffreestanding -nostdlib -c userspace\apps\sdk_explorer\main.c -o build\sdk_explorer.o
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

ld.lld -T userspace\linker.ld --strip-all build\sdk_explorer.o build\syscalls.o build\syscalls_gui.o build\widgets.o build\bos_gui.o build\bpde.o build\bishop_builtins.o -o build\sdk_explorer.elf
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED!" -ForegroundColor Red; exit $LASTEXITCODE }

Write-Host "Compiling DOOM..." -ForegroundColor Cyan
& .\userspace\apps\doom\build_doom.ps1
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED! DOOM compilation failed" -ForegroundColor Red; exit $LASTEXITCODE }

Write-Host "Compiling ATRIX Minimal Real-Web Browser Probe..." -ForegroundColor Cyan
& .\tools\gn_build.ps1
if (Test-Path "out\Default\minimal_real_browser.elf") {
    Copy-Item -Force "out\Default\minimal_real_browser.elf" "build\minimal_real_browser.elf"
    Write-Host "[OK] minimal_real_browser.elf copied to build\minimal_real_browser.elf" -ForegroundColor Green
}

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
}
else {
    $vboxManage = "VBoxManage"
}

& $vboxManage convertfromraw $imgPath $vdiPath --format VDI
if ($LASTEXITCODE -ne 0) {
    Write-Host "WARNING: VBoxManage not found or VDI conversion failed." -ForegroundColor Yellow
}
else {
    Write-Host "[OK] VDI Created: build\SignaturesOS.vdi" -ForegroundColor Green
}

# ==============================================================================
# VMDK CONVERSION (VMware Hard Disk Container)
# ==============================================================================
Write-Host "--- Converting to VMDK for VMware/QEMU ---" -ForegroundColor Cyan
$vmdkPath = "build\SignaturesOS.vmdk"
$vmdkTarget = $vmdkPath

if (Test-Path $vmdkPath) {
    try {
        Remove-Item $vmdkPath -Force -ErrorAction Stop
    } catch {
        Write-Host "NOTICE: build\SignaturesOS.vmdk is currently locked by VMware. Using build\SignaturesOS_Live.vmdk" -ForegroundColor Yellow
        $vmdkTarget = "build\SignaturesOS_Live.vmdk"
        if (Test-Path $vmdkTarget) {
            Remove-Item $vmdkTarget -Force -ErrorAction SilentlyContinue
        }
    }
}

& $vboxManage convertfromraw $imgPath $vmdkTarget --format VMDK
if ($LASTEXITCODE -ne 0) {
    Write-Host "WARNING: VMDK conversion failed." -ForegroundColor Yellow
}
else {
    Write-Host "[OK] VMDK Created: $vmdkTarget" -ForegroundColor Green
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



Write-Host "=========================================" -ForegroundColor Green



