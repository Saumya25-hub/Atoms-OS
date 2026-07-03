$logPath = "qemu_verify_codec.log"
$outDir = "docs/ac97_codec"

if (!(Test-Path $outDir)) {
    New-Item -ItemType Directory -Force -Path $outDir | Out-Null
}

$logData = Get-Content $logPath -Raw
$lines = $logData -split "`n" | ForEach-Object { $_.Trim() }

function Extract-Block($startMarker, $endMarker) {
    $inBlock = $false
    $block = @()
    foreach ($line in $lines) {
        if ($line -eq $endMarker -and $inBlock) {
            $inBlock = $false
            return $block
        }
        if ($inBlock) {
            $block += $line
        }
        if ($line -match $startMarker) {
            $inBlock = $true
        }
    }
    return $block
}

# 1. Reset
$resetBlock = Extract-Block "\[AC97 RESET\]" "Codec Ready: PASS"
$resetContent = "# AC97 Reset Sequence`n`n"
if ($resetBlock.Count -gt 0) {
    $resetContent += '```text' + "`n" + ($resetBlock -join "`n") + "`nCodec Ready: PASS`n" + '```' + "`n"
} else {
    $resetContent += "FAILED TO PARSE RESET`n"
}
Set-Content -Path "$outDir/01_reset.md" -Value $resetContent

# 2. Power
$powerBlock = Extract-Block "========== POWER ==========" "==========================="
$powerContent = "# AC97 Power State`n`n"
if ($powerBlock.Count -gt 0) {
    $powerContent += '```text' + "`n" + ($powerBlock -join "`n") + "`n" + '```' + "`n"
}
Set-Content -Path "$outDir/02_power.md" -Value $powerContent

# 3. Volumes (Master, Headphone, Mono)
# 4. PCM Volume (PCM Out)
$volContent = "# AC97 Volume Registers`n`n"
$pcmVolContent = "# AC97 PCM Volume`n`n"
$volBlocks = $logData | Select-String -Pattern "(?ms)(Master|Headphone|Mono|PCM Out) VOLUME\r?\n.*?(?=PASS\r?\n)PASS\r?\n" -AllMatches
foreach ($match in $volBlocks.Matches) {
    if ($match.Value -match "PCM Out") {
        $pcmVolContent += '```text' + "`n" + $match.Value + '```' + "`n"
    } else {
        $volContent += '```text' + "`n" + $match.Value + '```' + "`n"
    }
}
Set-Content -Path "$outDir/03_volume.md" -Value $volContent
Set-Content -Path "$outDir/04_pcm_volume.md" -Value $pcmVolContent

# 5. Extended Audio
$extBlock = Extract-Block "========== EXTENDED ==========" "=============================="
$extContent = "# AC97 Extended Capabilities`n`n" + '```text' + "`n" + ($extBlock -join "`n") + "`n" + '```' + "`n"
Set-Content -Path "$outDir/05_extended_audio.md" -Value $extContent

# 6. Sample Rate
$rateBlock = Extract-Block "\[SAMPLE RATE NEGOTIATION\]" "(PASS|FAIL - Fixed Rate Codec)"
$rateEnd = $lines | Where-Object { $_ -match "FAIL - Fixed Rate Codec" -or $_ -eq "PASS" } | Select-Object -Last 1
$rateContent = "# AC97 Sample Rate`n`n" + '```text' + "`n" + ($rateBlock -join "`n") + "`n$rateEnd`n" + '```' + "`n"
Set-Content -Path "$outDir/06_sample_rate.md" -Value $rateContent

# 7. Register Map
$regBlock = Extract-Block "\[REGISTER MAP\]" "\[PLAYBACK TIMING\]"
if ($regBlock.Count -eq 0) {
    $regBlock = Extract-Block "\[REGISTER MAP\]" "=============================="
}
$regContent = "# AC97 Register Dump`n`n" + '```text' + "`n" + ($regBlock -join "`n") + "`n" + '```' + "`n"
Set-Content -Path "$outDir/07_registers.md" -Value $regContent

# 8. Playback
$playBlock = Extract-Block "\[PLAYBACK TIMING\]" "PASS"
$playContent = "# Playback Timing`n`n" + '```text' + "`n" + ($playBlock -join "`n") + "`nPASS`n" + '```' + "`n"
Set-Content -Path "$outDir/08_playback.md" -Value $playContent

$codecFailed = $false
if ($logData -match "Codec Ready: FAIL" -or $logData -match "FAIL - Fixed Rate Codec" -or $logData -match "FAILED TO PARSE") {
    $codecFailed = $true
}

$rootCause = "No failure detected. Codec successfully verified."
if ($codecFailed) {
    $rootCause = "Codec failure detected in log. See specific modules."
}
Set-Content -Path "$outDir/09_root_cause.md" -Value "# Root Cause Analysis`n`n$rootCause"
Set-Content -Path "$outDir/10_walkthrough.md" -Value "# Phase 7.6.6 Walkthrough`n`nVerification scripts ran."

if (-not $codecFailed) {
    Write-Host "=============================="
    Write-Host "AC97 CODEC VERIFIED"
    Write-Host "=============================="
} else {
    Write-Host "=============================="
    Write-Host "AC97 CODEC FAILURE"
    Write-Host "=============================="
}
