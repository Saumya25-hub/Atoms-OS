@echo off
setlocal EnableDelayedExpansion

:: ============================================================================
:: ATOMS OS — REAL WINDOWS XP NATIVE NTFS VALIDATION DATASET BUILDER
:: ============================================================================
:: Target OS: Windows XP (CMD Native Syntax)
:: Purpose  : Populates a manually formatted disposable NTFS volume with a
::            deterministic validation dataset for ATOMS OS NTFS read testing.
:: ============================================================================

echo.
echo ============================================================================
echo   ATOMS OS -- WINDOWS XP NATIVE NTFS TEST DATASET PREPARATION
echo ============================================================================
echo.
echo   SAFETY WARNING:
echo   - This script WILL NOT format any disk.
echo   - This script WILL NOT alter partition tables or use diskpart.
echo   - Drive C:\ is STRICTLY FORBIDDEN and will be rejected immediately.
echo.

:: Prompt for Target Drive Letter
set /p TARGET_INPUT="Enter the target NTFS drive letter (e.g. E or E:): "

if "%TARGET_INPUT%"=="" (
    echo [ERROR] No drive letter entered. Aborting.
    goto :ABORT
)

:: Extract drive letter (first character)
set DRIVE_LET=%TARGET_INPUT:~0,1%

:: Check for C / c drive restriction
if /I "%DRIVE_LET%"=="C" (
    echo.
    echo [CRITICAL ERROR] Drive C:\ is prohibited! Cannot target system drive C.
    goto :ABORT
)

set TEST_DRIVE=%DRIVE_LET%:
set TEST_ROOT=%TEST_DRIVE%\ATOMS_TEST

:: Verify target drive existence
if not exist "%TEST_DRIVE%\" (
    echo.
    echo [ERROR] Drive %TEST_DRIVE% does not exist or is not mounted.
    echo Please format your virtual disk as NTFS and assign a valid drive letter first.
    goto :ABORT
)

echo.
echo ----------------------------------------------------------------------------
echo  TARGET CONFIRMATION REQUIRED
echo ----------------------------------------------------------------------------
echo  Target Drive  : %TEST_DRIVE%\
echo  Dataset Root  : %TEST_ROOT%\
echo.
echo  Confirm writing test dataset to %TEST_DRIVE%\ by typing YES in capital letters.
echo ----------------------------------------------------------------------------
echo.

set /p CONFIRM="Type YES to proceed: "

if not "%CONFIRM%"=="YES" (
    echo.
    echo [ABORTED] Confirmation was not 'YES'. No files or directories were created.
    goto :ABORT
)

echo.
echo [STATUS] Starting directory creation on %TEST_DRIVE%\ ...

:: Track counters for final summary
set DIR_COUNT=0
set FILE_COUNT=0
set LARGE_COUNT=0
set SKIP_COUNT=0

:: Create Directory Structure
call :MAKE_DIR "%TEST_ROOT%"
call :MAKE_DIR "%TEST_ROOT%\System"
call :MAKE_DIR "%TEST_ROOT%\System\Apps"
call :MAKE_DIR "%TEST_ROOT%\Nested"
call :MAKE_DIR "%TEST_ROOT%\Nested\Level1"
call :MAKE_DIR "%TEST_ROOT%\Nested\Level1\Level2"
call :MAKE_DIR "%TEST_ROOT%\Nested\Level1\Level2\Level3"
call :MAKE_DIR "%TEST_ROOT%\EmptyDirectory"
call :MAKE_DIR "%TEST_ROOT%\ManyFiles"

echo.
echo [STATUS] Populating text files and deterministic payloads...

:: Deterministic Text Files
call :WRITE_TEXT "%TEST_ROOT%\hello.txt" "ATOMS_OS_WINDOWS_XP_NATIVE_NTFS_TEST_OK"
call :WRITE_TEXT "%TEST_ROOT%\System\Apps\Test.txt" "ATOMS_OS_NTFS_WINDOWS_XP_PATH_RESOLUTION_OK"
call :WRITE_TEXT "%TEST_ROOT%\Nested\Level1\Level2\Level3\deep_test.txt" "ATOMS_OS_NTFS_DEEP_DIRECTORY_TRAVERSAL_OK"
call :WRITE_TEXT "%TEST_ROOT%\mixed_case.txt" "ATOMS_OS_NTFS_CASE_LOOKUP_TEST_OK"
call :WRITE_TEXT "%TEST_ROOT%\This_Is_A_Long_Windows_NTFS_Filename_For_ATOMS_OS_Testing.txt" "ATOMS_OS_LONG_FILENAME_TEST_OK"

:: Special NTFS Test Cases
call :WRITE_EMPTY "%TEST_ROOT%\empty_file.txt"
call :WRITE_TEXT "%TEST_ROOT%\File With Spaces.txt" "ATOMS_OS_FILENAME_WITH_SPACES_OK"
call :WRITE_TEXT "%TEST_ROOT%\MiXeD_CaSe_File.TxT" "ATOMS_OS_MIXED_CASE_FILENAME_OK"

:: Many Files Directory Enumeration (file01.txt .. file30.txt)
echo [STATUS] Creating 30 directory enumeration files in \ManyFiles\ ...
for /L %%I in (1,1,30) do (
    set NUM=0%%I
    set NUM=!NUM:~-2!
    call :WRITE_TEXT "%TEST_ROOT%\ManyFiles\file!NUM!.txt" "ATOMS_OS_DIRECTORY_ENTRY_!NUM!"
)

:: Binary / Large File Generation via FSUTIL (if available)
echo.
echo [STATUS] Checking for fsutil utility...
set FSUTIL_AVAIL=0
fsutil >nul 2>&1
if %ERRORLEVEL% equ 0 (
    set FSUTIL_AVAIL=1
    echo [STATUS] fsutil found. Generating binary stream test files...

    echo Creating medium_test.bin (64 KB)...
    fsutil file createnew "%TEST_ROOT%\medium_test.bin" 65536 >nul 2>&1
    if !ERRORLEVEL! equ 0 (
        set /a LARGE_COUNT+=1
        echo [OK] medium_test.bin (65536 bytes) created.
    ) else (
        set /a SKIP_COUNT+=1
        echo [WARNING] Failed to create medium_test.bin with fsutil.
    )

    echo Creating large_test.bin (1 MB)...
    fsutil file createnew "%TEST_ROOT%\large_test.bin" 1048576 >nul 2>&1
    if !ERRORLEVEL! equ 0 (
        set /a LARGE_COUNT+=1
        echo [OK] large_test.bin (1048576 bytes) created.
    ) else (
        set /a SKIP_COUNT+=1
        echo [WARNING] Failed to create large_test.bin with fsutil.
    )
) else (
    set /a SKIP_COUNT+=2
    echo [WARNING] fsutil not found on this Windows XP environment.
    echo Skipping 64KB and 1MB binary stream files. Text file test cases remain fully valid.
)

:: Create Validation Manifest
echo.
echo [STATUS] Writing manifest file \ATOMS_NTFS_TEST_MANIFEST.txt ...

set MANIFEST=%TEST_ROOT%\ATOMS_NTFS_TEST_MANIFEST.txt

echo ============================================================ > "%MANIFEST%"
echo  ATOMS OS NTFS NATIVE WINDOWS XP VALIDATION MANIFEST >> "%MANIFEST%"
echo ============================================================ >> "%MANIFEST%"
echo WINDOWS SOURCE  : Windows XP (CMD Native Script) >> "%MANIFEST%"
echo FILESYSTEM      : Real Windows NTFS >> "%MANIFEST%"
echo TARGET DRIVE    : %TEST_DRIVE%\ >> "%MANIFEST%"
echo DATASET ROOT    : %TEST_ROOT%\ >> "%MANIFEST%"
echo CREATION METHOD :prepare_atoms_ntfs_test.bat >> "%MANIFEST%"
echo. >> "%MANIFEST%"
echo [EXPECTED DIRECTORIES] >> "%MANIFEST%"
echo - \ATOMS_TEST\ >> "%MANIFEST%"
echo - \ATOMS_TEST\System\ >> "%MANIFEST%"
echo - \ATOMS_TEST\System\Apps\ >> "%MANIFEST%"
echo - \ATOMS_TEST\Nested\Level1\Level2\Level3\ >> "%MANIFEST%"
echo - \ATOMS_TEST\EmptyDirectory\ >> "%MANIFEST%"
echo - \ATOMS_TEST\ManyFiles\ >> "%MANIFEST%"
echo. >> "%MANIFEST%"
echo [EXPECTED TEXT FILES AND PAYLOADS] >> "%MANIFEST%"
echo - \ATOMS_TEST\hello.txt : ATOMS_OS_WINDOWS_XP_NATIVE_NTFS_TEST_OK >> "%MANIFEST%"
echo - \ATOMS_TEST\System\Apps\Test.txt : ATOMS_OS_NTFS_WINDOWS_XP_PATH_RESOLUTION_OK >> "%MANIFEST%"
echo - \ATOMS_TEST\Nested\Level1\Level2\Level3\deep_test.txt : ATOMS_OS_NTFS_DEEP_DIRECTORY_TRAVERSAL_OK >> "%MANIFEST%"
echo - \ATOMS_TEST\mixed_case.txt : ATOMS_OS_NTFS_CASE_LOOKUP_TEST_OK >> "%MANIFEST%"
echo - \ATOMS_TEST\This_Is_A_Long_Windows_NTFS_Filename_For_ATOMS_OS_Testing.txt : ATOMS_OS_LONG_FILENAME_TEST_OK >> "%MANIFEST%"
echo - \ATOMS_TEST\empty_file.txt : (0 Bytes) >> "%MANIFEST%"
echo - \ATOMS_TEST\File With Spaces.txt : ATOMS_OS_FILENAME_WITH_SPACES_OK >> "%MANIFEST%"
echo - \ATOMS_TEST\MiXeD_CaSe_File.TxT : ATOMS_OS_MIXED_CASE_FILENAME_OK >> "%MANIFEST%"
echo - \ATOMS_TEST\ManyFiles\file01.txt..file30.txt : ATOMS_OS_DIRECTORY_ENTRY_01..30 >> "%MANIFEST%"
echo. >> "%MANIFEST%"
echo [BINARY STREAM FILES] >> "%MANIFEST%"
if !FSUTIL_AVAIL! equ 1 (
    echo - \ATOMS_TEST\medium_test.bin : 65536 bytes (fsutil zero-fill stream) >> "%MANIFEST%"
    echo - \ATOMS_TEST\large_test.bin  : 1048576 bytes (fsutil zero-fill stream) >> "%MANIFEST%"
) else (
    echo - Binary files skipped (fsutil unavailable) >> "%MANIFEST%"
)
echo. >> "%MANIFEST%"
echo [STATUS SUMMARY] >> "%MANIFEST%"
echo Directories Created : !DIR_COUNT! >> "%MANIFEST%"
echo Text Files Created  : !FILE_COUNT! >> "%MANIFEST%"
echo Large Files Created : !LARGE_COUNT! >> "%MANIFEST%"
echo Skipped Operations  : !SKIP_COUNT! >> "%MANIFEST%"
echo RESULT             : SUCCESS >> "%MANIFEST%"

set /a FILE_COUNT+=1

:: Final Summary Output
echo.
echo ============================================================================
echo  ATOMS OS NTFS TEST DATASET READY
echo ============================================================================
echo  Target Drive        : %TEST_DRIVE%\
echo  Dataset Root        : %TEST_ROOT%\
echo  Directories Created : %DIR_COUNT%
echo  Text Files Created  : %FILE_COUNT% (includes manifest & ManyFiles)
echo  Large Files Created : %LARGE_COUNT%
echo  Skipped Operations  : %SKIP_COUNT%
echo  Overall Status      : SUCCESS
echo ============================================================================
echo.
echo NEXT INSTRUCTIONS FOR USER:
echo   1. Open Windows Explorer on Windows XP and verify %TEST_ROOT%\ contains all files.
echo   2. Shut down the Windows XP Virtual Machine completely.
echo   3. DO NOT format or delete the virtual disk.
echo   4. Attach or pass this virtual disk to ATOMS OS QEMU execution.
echo.
goto :END

:: Helper Subroutines
:MAKE_DIR
mkdir "%~1" 2>nul
set /a DIR_COUNT+=1
goto :EOF

:WRITE_TEXT
echo %~2>"%~1"
set /a FILE_COUNT+=1
goto :EOF

:WRITE_EMPTY
type nul > "%~1"
set /a FILE_COUNT+=1
goto :EOF

:ABORT
echo.
echo Script aborted. No changes made.
exit /b 1

:END
endlocal
exit /b 0
