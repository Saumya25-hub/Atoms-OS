@echo off
title ATOMS MATRIX DEBUG ENGINE (AMDE) V2.0
cd /d "d:\Signatures_OS"
echo =====================================================================
echo   ATOMS MATRIX DEBUG ENGINE (AMDE) V2.0 -- MOTHER FORENSIC GUI
echo =====================================================================
echo Starting GUI Debugger...
python tools\atoms_control_center.py %*
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo [ERROR] AMDE Debug Tool exited with code %ERRORLEVEL%.
    pause
)
