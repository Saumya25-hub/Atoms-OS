# ===========================================================================
# ATOMS OS — Phase 7R Real Windows NTFS Media Creator & Manifest Generator
# ===========================================================================
# Creates a 256MB VHD, attaches it safely, formats it with Windows native NTFS,
# populates a deterministic real-world test dataset, computes SHA-256 manifest,
# safely detaches the VHD, and converts to a raw disk image for QEMU attachment.
# ===========================================================================

$ErrorActionPreference = "Stop"

$buildDir  = Join-Path $PSScriptRoot "..\build"
$vhdPath   = [System.IO.Path]::GetFullPath((Join-Path $buildDir "ntfs_real_test.vhd"))
$rawPath   = [System.IO.Path]::GetFullPath((Join-Path $buildDir "ntfs_real_test.raw"))
$manifestPath = [System.IO.Path]::GetFullPath((Join-Path $buildDir "ntfs_real_test_manifest.txt"))

if (-not (Test-Path $buildDir)) {
    New-Item -ItemType Directory -Path $buildDir | Out-Null
}

Write-Host "=========================================" -ForegroundColor Cyan
Write-Host " [PHASE 7R] Real Windows NTFS Disk Build" -ForegroundColor Cyan
Write-Host "=========================================" -ForegroundColor Cyan
Write-Host "VHD Path     : $vhdPath"
Write-Host "Raw Path     : $rawPath"
Write-Host "Manifest Path: $manifestPath"

# Step 1: Clean up previous VHD / Detach if mounted
if (Test-Path $vhdPath) {
    Write-Host "[1/6] Cleaning up old VHD image..." -ForegroundColor Yellow
    $detachScript = @"
select vdisk file="$vhdPath"
detach vdisk
"@
    $detachTmp = [System.IO.Path]::GetTempFileName()
    Set-Content -Path $detachTmp -Value $detachScript
    diskpart /s $detachTmp | Out-Null
    Remove-Item -Path $detachTmp -Force -ErrorAction SilentlyContinue
    Start-Sleep -Seconds 1
    Remove-Item -Path $vhdPath -Force -ErrorAction SilentlyContinue
}

# Find an unused drive letter
$usedLetters = (Get-Volume).DriveLetter | Where-Object { $_ }
$targetLetter = [char[]](69..90) | Where-Object { $usedLetters -notcontains $_ } | Select-Object -First 1
if (-not $targetLetter) {
    Write-Error "No available drive letter found!"
}

Write-Host "[2/6] Creating & Formatting Genuine Windows NTFS VHD (Drive ${targetLetter}:)..." -ForegroundColor Cyan

$createScript = @"
create vdisk file="$vhdPath" maximum=256 type=fixed
select vdisk file="$vhdPath"
attach vdisk
convert mbr
create partition primary
format fs=ntfs label="NTFS_REAL" quick
assign letter=$targetLetter
"@

$scriptTmp = [System.IO.Path]::GetTempFileName()
Set-Content -Path $scriptTmp -Value $createScript

try {
    diskpart /s $scriptTmp | Out-Null
} finally {
    Remove-Item -Path $scriptTmp -Force -ErrorAction SilentlyContinue
}

Start-Sleep -Seconds 2

# Safety Verification: Ensure drive letter is attached to non-boot virtual drive
$mountedVol = Get-Volume -DriveLetter $targetLetter -ErrorAction SilentlyContinue
if (-not $mountedVol) {
    Write-Error "Failed to locate mounted NTFS volume on drive ${targetLetter}:"
}

$mountedPartition = Get-Partition -DriveLetter $targetLetter -ErrorAction SilentlyContinue
if ($mountedPartition.IsBoot -or $mountedPartition.IsSystem) {
    Write-Error "SAFETY VIOLATION: Target drive letter ${targetLetter}: is marked as Boot/System!"
}

Write-Host "[3/6] Populating Deterministic Real-World NTFS Dataset..." -ForegroundColor Green

$driveRoot = "${targetLetter}:\"
$sysAppsDir = Join-Path $driveRoot "System\Apps"
$nestedDir  = Join-Path $sysAppsDir "Nested\Deep"
$dirTestDir = Join-Path $sysAppsDir "dir_test"

New-Item -ItemType Directory -Path $sysAppsDir -Force | Out-Null
New-Item -ItemType Directory -Path $nestedDir -Force | Out-Null
New-Item -ItemType Directory -Path $dirTestDir -Force | Out-Null

# 1. hello.txt
$helloPath = Join-Path $sysAppsDir "hello.txt"
[System.IO.File]::WriteAllText($helloPath, "ATOMS_NTFS_REAL_MEDIA_OK", [System.Text.Encoding]::ASCII)

# 2. small.txt
$smallPath = Join-Path $sysAppsDir "small.txt"
[System.IO.File]::WriteAllText($smallPath, "SMALL_RESIDENT_NTFS_PAYLOAD", [System.Text.Encoding]::ASCII)

# 3. medium.bin (8KB binary file)
$mediumPath = Join-Path $sysAppsDir "medium.bin"
$medBytes = New-Object byte[] 8192
for ($i = 0; $i -lt 8192; $i++) { $medBytes[$i] = [byte]($i % 256) }
[System.IO.File]::WriteAllBytes($mediumPath, $medBytes)

# 4. large.bin (1MB binary file spanning multiple clusters)
$largePath = Join-Path $sysAppsDir "large.bin"
$largeBytes = New-Object byte[] 1048576
for ($i = 0; $i -lt 1048576; $i++) { $largeBytes[$i] = [byte](( $i * 7 + 13 ) % 256) }
[System.IO.File]::WriteAllBytes($largePath, $largeBytes)

# 5. zeroes.bin (64KB zero filled file)
$zeroesPath = Join-Path $sysAppsDir "zeroes.bin"
$zeroBytes = New-Object byte[] 65536
[System.IO.File]::WriteAllBytes($zeroesPath, $zeroBytes)

# 6. Deep nested file
$deepPath = Join-Path $nestedDir "real_test.txt"
[System.IO.File]::WriteAllText($deepPath, "NESTED_DEEP_DIRECTORY_FILE_OK", [System.Text.Encoding]::ASCII)

# 7. Long filename file
$longPath = Join-Path $sysAppsDir "long_filename_test_for_mft_record_validation.txt"
[System.IO.File]::WriteAllText($longPath, "LONG_FILENAME_PAYLOAD_OK", [System.Text.Encoding]::ASCII)

# 8. Directory scaling files
for ($f = 1; $f -le 20; $f++) {
    $fPath = Join-Path $dirTestDir ("file{0:D2}.txt" -f $f)
    [System.IO.File]::WriteAllText($fPath, "DIR_TEST_FILE_$f", [System.Text.Encoding]::ASCII)
}

Write-Host "[4/6] Generating Authoritative Host Manifest ($manifestPath)..." -ForegroundColor Cyan

$manifestLines = @()
$manifestLines += "# ATOMS OS Real-Media NTFS Manifest"
$manifestLines += "# Generated: $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')"
$manifestLines += "# Drive: ${targetLetter}:"
$manifestLines += ""

$allFiles = Get-ChildItem -Path $driveRoot -Recurse -File
foreach ($file in $allFiles) {
    $relPath = $file.FullName.Substring($driveRoot.Length).Replace("\", "/")
    $hash = (Get-FileHash -Path $file.FullName -Algorithm SHA256).Hash
    $size = $file.Length
    $manifestLines += "$relPath | Size: $size | SHA256: $hash"
}

Set-Content -Path $manifestPath -Value $manifestLines

Write-Host "[5/6] Flushing & Safely Detaching VHD..." -ForegroundColor Yellow

$detachScript2 = @"
select vdisk file="$vhdPath"
detach vdisk
"@
$detachTmp2 = [System.IO.Path]::GetTempFileName()
Set-Content -Path $detachTmp2 -Value $detachScript2
diskpart /s $detachTmp2 | Out-Null
Remove-Item -Path $detachTmp2 -Force -ErrorAction SilentlyContinue

Start-Sleep -Seconds 2

Write-Host "[6/6] Converting VHD to Raw Image ($rawPath)..." -ForegroundColor Cyan
if (Test-Path $rawPath) { Remove-Item -Path $rawPath -Force }

qemu-img convert -f vpc -O raw "$vhdPath" "$rawPath"

Write-Host "=========================================" -ForegroundColor Green
Write-Host " REAL WINDOWS NTFS TEST MEDIA READY!" -ForegroundColor Green
Write-Host " Raw Image Size: $((Get-Item $rawPath).Length) bytes" -ForegroundColor Green
Write-Host " Manifest File : $manifestPath" -ForegroundColor Green
Write-Host "=========================================" -ForegroundColor Green
