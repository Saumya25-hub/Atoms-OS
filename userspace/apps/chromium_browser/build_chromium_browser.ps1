# =====================================================================
# ATOMS OS — FULL CHROMIUM DESKTOP APPLICATION BUILD SCRIPT
# =====================================================================
# Compiles userspace/apps/chromium_browser/ into build/chromium_browser.elf
# Powered by genuine upstream Chromium //base, Clang++, LLD, libc++, APAL
# =====================================================================

$ErrorActionPreference = "Stop"

Write-Host "=====================================================================" -ForegroundColor Cyan
Write-Host "Compiling Chromium Desktop Browser (genuine //base integrated)..." -ForegroundColor Cyan
Write-Host "=====================================================================" -ForegroundColor Cyan

$srcDir = "userspace\apps\chromium_browser"
$buildDir = "build"

# 1. Compile C++ Sources
$cppFiles = @(
    "$srcDir\src\chromium_window.cpp",
    "$srcDir\src\atoms_chromium_base_compat.cpp",
    "$srcDir\src\main.cpp"
)

$objFiles = @()

foreach ($cpp in $cppFiles) {
    $obj = "$buildDir\" + [System.IO.Path]::GetFileNameWithoutExtension($cpp) + ".o"
    $objFiles += $obj
    Write-Host "Compiling $cpp -> $obj" -ForegroundColor Yellow
    clang++ -std=c++23 -target x86_64-unknown-none-elf `
        -nostdinc -nostdinc++ -ffreestanding -fno-stack-protector -mno-red-zone `
        -fno-pic -fno-pie -mcmodel=small -fno-exceptions -fno-rtti -O2 `
        -DOS_ATOMS=1 -D_GNU_SOURCE -D__ATOMS__ `
        -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS `
        -D_LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_NONE -DDCHECK_ALWAYS_ON=1 `
        -I. -I$srcDir\include -Iatoms\userspace\runtime\include `
        -Iatoms\userspace\apal\include -Ithird_party\llvm\libcxxabi\include `
        -Ithird_party\llvm\libcxx\include -Ithird_party\musl\arch\x86_64 `
        -Ithird_party\musl\arch\generic -Ithird_party\musl\include `
        -Ithird_party\chromium\src `
        -Ithird_party\chromium\src\third_party\abseil-cpp `
        -Ithird_party\chromium\src\third_party\ipcz\src `
        -Ithird_party\chromium\src\third_party\ipcz\include `
        -Ithird_party\chromium\src\third_party\perfetto\include `
        -Ithird_party\chromium\src\base\allocator\partition_allocator\src `
        -Iout\atoms\gen `
        -Iout\atoms\gen\base\allocator\partition_allocator\src `
        -c $cpp -o $obj
    if ($LASTEXITCODE -ne 0) {
        Write-Host "COMPILATION FAILED on $cpp" -ForegroundColor Red
        exit $LASTEXITCODE
    }
}

# 2. Link into Standalone ELF with Genuine Upstream Chromium Base & Mojo
$targetElf = "$buildDir\chromium_browser.elf"
Write-Host "Linking $targetElf with genuine upstream Chromium //base and //mojo..." -ForegroundColor Yellow

ld.lld -T userspace\linker.ld `
    atoms\userspace\runtime\crt0.o `
    $objFiles `
    out\atoms\obj\mojo\public\cpp\system\libmojo_public_system_cpp.a `
    out\atoms\obj\mojo\public\c\system\libmojo_public_system.a `
    out\atoms\obj\mojo\public\cpp\platform\libmojo_cpp_platform.a `
    out\atoms\obj\mojo\core\embedder\libmojo_core_embedder.a `
    out\atoms\obj\mojo\core\embedder\libmojo_core_embedder_features.a `
    out\atoms\obj\mojo\libmojo_core_all.a `
    out\atoms\obj\base\libatoms_base.a `
    out\atoms\obj\base\third_party\double_conversion\libdouble_conversion.a `
    atoms\userspace\apal\libapal.a `
    atoms\userspace\runtime\libatoms_cpp.a `
    atoms\userspace\runtime\libatoms_c.a `
    -o $targetElf

if ($LASTEXITCODE -ne 0) {
    Write-Host "LINK FAILED for $targetElf" -ForegroundColor Red
    exit $LASTEXITCODE
}

Write-Host "[OK] Chromium Desktop Browser Built Successfully: $targetElf" -ForegroundColor Green
Write-Host "=====================================================================" -ForegroundColor Cyan
