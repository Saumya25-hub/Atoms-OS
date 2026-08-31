# ATOMS OS -- Chromium GN and Ninja Build Driver (tools/gn_build.ps1)
# Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari

Write-Host "=========================================================" -ForegroundColor Cyan
Write-Host " ATOMS OS -- Chromium GN + Ninja + Clang Toolchain Build  " -ForegroundColor Cyan
Write-Host "=========================================================" -ForegroundColor Cyan

# Step 1: Run GN Meta-Build Generator
Write-Host "[1/2] Generating Ninja build files via GN (tools/gn.exe)..." -ForegroundColor Yellow
& .\tools\gn.exe gen out\Default
if ($LASTEXITCODE -ne 0) {
    Write-Host "[ERROR] GN generation failed with code $LASTEXITCODE" -ForegroundColor Red
    exit $LASTEXITCODE
}

# Step 2: Run Ninja Multi-Threaded Build
Write-Host "[2/2] Compiling and linking targets via Ninja (tools/ninja.exe)..." -ForegroundColor Yellow
& .\tools\ninja.exe -C out\Default
if ($LASTEXITCODE -ne 0) {
    Write-Host "[ERROR] Ninja build failed with code $LASTEXITCODE" -ForegroundColor Red
    exit $LASTEXITCODE
}

Write-Host "=========================================================" -ForegroundColor Green
Write-Host " BUILD SUCCESSFUL! Targets generated in out/Default/     " -ForegroundColor Green
Write-Host " - out/Default/atoms_c_test.elf                          " -ForegroundColor Green
Write-Host " - out/Default/atoms_cpp_test.elf                        " -ForegroundColor Green
Write-Host " - out/Default/atoms_generated_test.elf                  " -ForegroundColor Green
Write-Host " - out/Default/chromium_toolchain_verify.elf             " -ForegroundColor Green
Write-Host "=========================================================" -ForegroundColor Green
