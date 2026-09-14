Write-Host "===================================================" -ForegroundColor Cyan
Write-Host "  ATOMS OS -- CONFIGURING FIREWALL FOR PXE BOOT" -ForegroundColor Green
Write-Host "===================================================" -ForegroundColor Cyan

try {
    netsh advfirewall firewall add rule name="ATOMS_PXE_DHCP" dir=in action=allow protocol=UDP localport=67 profile=any
    netsh advfirewall firewall add rule name="ATOMS_PXE_TFTP" dir=in action=allow protocol=UDP localport=69 profile=any
    netsh advfirewall firewall add rule name="ATOMS_PXE_TELEMETRY" dir=in action=allow protocol=UDP localport=9998,9999 profile=any
    netsh advfirewall set allprofiles state off
    Write-Host "SUCCESS: Windows Defender Firewall disabled and PXE rules added!" -ForegroundColor Green
} catch {
    Write-Host "ERROR: $_" -ForegroundColor Red
}

Start-Sleep -Seconds 3
