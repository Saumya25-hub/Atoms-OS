@echo off
echo ===================================================
echo   ATOMS OS -- OPENING PXE PORTS IN WINDOWS FIREWALL
echo ===================================================
echo Adding inbound rules for UDP ports 67, 69, 9998, 9999...
netsh advfirewall firewall add rule name="ATOMS_PXE_DHCP" dir=in action=allow protocol=UDP localport=67 profile=any
netsh advfirewall firewall add rule name="ATOMS_PXE_TFTP" dir=in action=allow protocol=UDP localport=69 profile=any
netsh advfirewall firewall add rule name="ATOMS_PXE_TELEMETRY" dir=in action=allow protocol=UDP localport=9998,9999 profile=any
echo Disabling Windows Firewall across all profiles for PXE testing...
netsh advfirewall set allprofiles state off
echo ===================================================
echo   SUCCESS: FIREWALL IS NOW OPEN FOR PXE BOOT!
echo ===================================================
timeout /t 3
