# =============================================================================
# ATOMS OS — Reproducible FreeBSD Guest Mountroot / VirtIO Test Script
# Copyright (c) 2026 ATOMS OS Project / Saumya Chaudhari
#
# Deterministic VM Test Harness:
#   ATOMS BOS Kernel -> Hypervisor -> FreeBSD guest -> VirtIO-BLK -> mountroot
# =============================================================================

param (
    [switch]$SkipBuild = $false,
    [int]$TimeoutSeconds = 90,
    [string]$LogFile = ""
)

$ErrorActionPreference = "Stop"

Write-Host "=================================================================" -ForegroundColor Cyan
Write-Host " ATOMS OS -- REPRODUCIBLE VMX/FREEBSD MOUNTROOT TEST RUNNER" -ForegroundColor Cyan
Write-Host "=================================================================" -ForegroundColor Cyan

# 0. Set up Paths
$repoRoot = "D:\Signatures_OS"
Set-Location $repoRoot

$qemuExe = "D:\OS-QEMU-EMU\qemu\qemu-system-x86_64.exe"
$ovmfCode = "D:\OS-QEMU-EMU\qemu\share\edk2-x86_64-code.fd"
$imgFile = "build\atoms_uefi_test.img"
$timestamp = Get-Date -Format "yyyyMMdd_HHmmss"

if ([string]::IsNullOrEmpty($LogFile)) {
    $LogFile = "build\freebsd_mountroot_test_$timestamp.log"
}

# Ensure destination directory exists
$logDir = [System.IO.Path]::GetDirectoryName($LogFile)
if (-not [string]::IsNullOrEmpty($logDir) -and -not (Test-Path $logDir)) {
    New-Item -ItemType Directory -Path $logDir -Force | Out-Null
}

if (-not (Test-Path $qemuExe)) {
    Write-Error "QEMU executable not found at: $qemuExe"
    exit 1
}
if (-not (Test-Path $ovmfCode)) {
    Write-Error "OVMF firmware not found at: $ovmfCode"
    exit 1
}

# 1. Build Phase (if not skipped)
if (-not $SkipBuild) {
    Write-Host "[1/3] Building Kernel & UEFI Artifacts via build.ps1..." -ForegroundColor Yellow
    & powershell -ExecutionPolicy Bypass -File build.ps1
    if ($LASTEXITCODE -ne 0) {
        Write-Error "build.ps1 failed with exit code $LASTEXITCODE"
        exit $LASTEXITCODE
    }

    Write-Host "      Generating GPT test image: $imgFile..." -ForegroundColor Yellow
    & .\build\gpt_image_builder.exe build\BOOTX64.EFI build\kernel.bin $imgFile
    if ($LASTEXITCODE -ne 0) {
        Write-Error "gpt_image_builder.exe failed with exit code $LASTEXITCODE"
        exit $LASTEXITCODE
    }
} else {
    Write-Host "[1/3] Skipping build phase (using existing image: $imgFile)..." -ForegroundColor Yellow
}

if (-not (Test-Path $imgFile)) {
    Write-Error "Target UEFI disk image not found at: $imgFile"
    exit 1
}

# Remove stale log if exists
if (Test-Path $LogFile) {
    Remove-Item $LogFile -Force
}

# 2. Launch QEMU with Deterministic Configuration
Write-Host "[2/3] Launching QEMU Pure UEFI Test VM..." -ForegroundColor Yellow
Write-Host "      Command: $qemuExe -machine q35 -cpu max,vmx=on -m 4096M" -ForegroundColor DarkGray
Write-Host "      Firmware: $ovmfCode" -ForegroundColor DarkGray
Write-Host "      Disk: $imgFile" -ForegroundColor DarkGray
Write-Host "      Serial Log: $LogFile" -ForegroundColor DarkGray
Write-Host "      Timeout: $TimeoutSeconds seconds" -ForegroundColor DarkGray

$qemuArgs = @(
    "-machine", "q35",
    "-cpu", "max,vmx=on",
    "-m", "4096M",
    "-drive", "if=pflash,format=raw,readonly=on,file=$ovmfCode",
    "-drive", "file=$imgFile,format=raw",
    "-device", "qemu-xhci",
    "-device", "usb-kbd",
    "-device", "usb-mouse",
    "-netdev", "user,id=net0",
    "-device", "e1000,netdev=net0",
    "-serial", "file:$LogFile",
    "-display", "none",
    "-no-reboot"
)

$psi = New-Object System.Diagnostics.ProcessStartInfo
$psi.FileName = $qemuExe
$psi.Arguments = $qemuArgs -join " "
$psi.UseShellExecute = $false
$psi.CreateNoWindow = $true

$process = [System.Diagnostics.Process]::Start($psi)

# 3. Stream Live Serial Output & Monitor for Milestones
Write-Host "[3/3] Streaming Serial COM1 Log (monitoring for mountroot / VirtIO)..." -ForegroundColor Yellow
$startTime = [System.Diagnostics.Stopwatch]::StartNew()
$readerPos = 0
$mountrootObserved = $false
$mountrootTime = $null

try {
    while ($startTime.Elapsed.TotalSeconds -lt $TimeoutSeconds -and -not $process.HasExited) {
        if (Test-Path $LogFile) {
            try {
                $stream = [System.IO.File]::Open($LogFile, [System.IO.FileMode]::Open, [System.IO.FileAccess]::Read, [System.IO.FileShare]::ReadWrite)
                if ($stream.Length -gt $readerPos) {
                    $stream.Seek($readerPos, [System.IO.SeekOrigin]::Begin) | Out-Null
                    $reader = New-Object System.IO.StreamReader($stream, [System.Text.Encoding]::UTF8)
                    $chunk = $reader.ReadToEnd()
                    $readerPos = $stream.Position
                    $reader.Close()
                    $stream.Close()

                    if (-not [string]::IsNullOrEmpty($chunk)) {
                        [Console]::Write($chunk)

                        # Mirror to Mother Debugger UDP 9999 if active
                        try {
                            $udpClient = New-Object System.Net.Sockets.UdpClient
                            $chunkBytes = [System.Text.Encoding]::ASCII.GetBytes($chunk)
                            $udpClient.Send($chunkBytes, $chunkBytes.Length, "127.0.0.1", 9999) | Out-Null
                            $udpClient.Close()
                        } catch {}

                        if ($chunk -match "Trying to mount root|vfs_mountroot|vtbd0") {
                            if (-not $mountrootObserved) {
                                $mountrootObserved = $true
                                $mountrootTime = $startTime.Elapsed.TotalSeconds
                                Write-Host "`n>>> [MARKER DETECTED] Mountroot activity observed at ${mountrootTime}s! <<<`n" -ForegroundColor Green
                            }
                        }
                    }
                } else {
                    $stream.Close()
                }
            } catch {
                # File access retry
            }
        }

        # If mountroot was observed, give it 15 more seconds to capture all retry/error logs
        if ($mountrootObserved -and ($startTime.Elapsed.TotalSeconds - $mountrootTime) -gt 15) {
            Write-Host "`n>>> Sufficient mountroot evidence collected post-marker. Stopping VM. <<<" -ForegroundColor Green
            break
        }

        Start-Sleep -Milliseconds 300
    }
} finally {
    if (-not $process.HasExited) {
        Write-Host "`nStopping QEMU process (PID $($process.Id))..." -ForegroundColor DarkYellow
        $process.Kill()
        $process.WaitForExit(3000)
    }
}

Write-Host "`n=================================================================" -ForegroundColor Cyan
Write-Host " FORENSIC EVIDENCE SUMMARY" -ForegroundColor Cyan
Write-Host "=================================================================" -ForegroundColor Cyan
Write-Host "Log File : $LogFile"

if (Test-Path $LogFile) {
    $fullContent = [System.IO.File]::ReadAllText($LogFile, [System.Text.Encoding]::UTF8)
    Write-Host "Log Size : $($fullContent.Length) bytes"
    
    $hasVtbd0 = ($fullContent -match "vtbd0")
    $hasMountroot = ($fullContent -match "Trying to mount root")
    $hasVirtioTrace = ($fullContent -match "\[VIRTIO-BLK TRACE\]")
    $hasQueueNotify = ($fullContent -match "QUEUE_NOTIFY")
    $hasReadReq = ($fullContent -match "READ/T_IN|Backend READ")

    Write-Host "1. /dev/vtbd0 Present in Log   : $(if ($hasVtbd0) { 'YES' } else { 'NO' })"
    Write-Host "2. Mountroot Stage Reached     : $(if ($hasMountroot) { 'YES' } else { 'NO' })"
    Write-Host "3. VirtIO-BLK Trace Triggered  : $(if ($hasVirtioTrace) { 'YES' } else { 'NO' })"
    Write-Host "4. QUEUE_NOTIFY Received       : $(if ($hasQueueNotify) { 'YES' } else { 'NO' })"
    Write-Host "5. Backend READ Request Seen   : $(if ($hasReadReq) { 'YES' } else { 'NO' })"
} else {
    Write-Host "ERROR: Serial log was not generated!" -ForegroundColor Red
}

Write-Host "=================================================================" -ForegroundColor Cyan
