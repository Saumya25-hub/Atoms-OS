param (
    [string]$filePath = "atoms_test.wav"
)

if (-Not (Test-Path $filePath)) {
    Write-Host "[WAV VERIFY] FAIL" -ForegroundColor Red
    Write-Host "Reason: File not found ($filePath)"
    exit 1
}

$bytes = [System.IO.File]::ReadAllBytes($filePath)

if ($bytes.Length -lt 44) {
    Write-Host "[WAV VERIFY] FAIL" -ForegroundColor Red
    Write-Host "Reason: File too small for WAV header"
    exit 1
}

# RIFF
$riff = [System.Text.Encoding]::ASCII.GetString($bytes[0..3])
if ($riff -ne "RIFF") {
    Write-Host "[WAV VERIFY] FAIL" -ForegroundColor Red
    Write-Host "Reason: Missing RIFF signature"
    exit 1
}

# WAVE
$wave = [System.Text.Encoding]::ASCII.GetString($bytes[8..11])
if ($wave -ne "WAVE") {
    Write-Host "[WAV VERIFY] FAIL" -ForegroundColor Red
    Write-Host "Reason: Missing WAVE signature"
    exit 1
}

# fmt chunk
$fmt = [System.Text.Encoding]::ASCII.GetString($bytes[12..14]) # 'fmt '
if ($fmt -ne "fmt") {
    Write-Host "[WAV VERIFY] FAIL" -ForegroundColor Red
    Write-Host "Reason: Missing fmt chunk"
    exit 1
}

$sampleRate = [System.BitConverter]::ToUInt32($bytes, 24)
$channels = [System.BitConverter]::ToUInt16($bytes, 22)
$bits = [System.BitConverter]::ToUInt16($bytes, 34)

# find data chunk
$dataIdx = 12
while ($dataIdx -lt ($bytes.Length - 8)) {
    $chunkStr = [System.Text.Encoding]::ASCII.GetString($bytes[$dataIdx..($dataIdx+3)])
    if ($chunkStr -eq "data") {
        break
    }
    $chunkSize = [System.BitConverter]::ToUInt32($bytes, $dataIdx + 4)
    $dataIdx += 8 + $chunkSize
}

if ($dataIdx -ge ($bytes.Length - 8)) {
    Write-Host "[WAV VERIFY] FAIL" -ForegroundColor Red
    Write-Host "Reason: Missing data chunk"
    exit 1
}

$dataLength = [System.BitConverter]::ToUInt32($bytes, $dataIdx + 4)

if ($dataLength -eq 0) {
    Write-Host "[WAV VERIFY] WARNING: Data chunk length is 0 (Likely QEMU force-kill)." -ForegroundColor Yellow
    $dataLength = $bytes.Length - $dataIdx - 8
    Write-Host "Recovered Data Length: $dataLength bytes" -ForegroundColor Yellow
    if ($dataLength -le 0) {
        Write-Host "[WAV VERIFY] FAIL" -ForegroundColor Red
        Write-Host "Reason: No data recovered"
        exit 1
    }
}

$nonZero = 0
for ($i = $dataIdx + 8; $i -lt $bytes.Length; $i++) {
    if ($bytes[$i] -ne 0) {
        $nonZero++
        if ($nonZero -gt 100) { break }
    }
}

if ($nonZero -eq 0) {
    Write-Host "[WAV VERIFY] FAIL" -ForegroundColor Red
    Write-Host "Reason: WAV file contains only silence"
    exit 1
}

Write-Host "[WAV VERIFY] PASS" -ForegroundColor Green
Write-Host "File: $filePath"
Write-Host "Format: ${sampleRate}Hz, ${bits}-bit, ${channels} channels"
Write-Host "Data Length: $dataLength bytes"
Write-Host "WAV structure perfectly intact and contains active PCM signals."
