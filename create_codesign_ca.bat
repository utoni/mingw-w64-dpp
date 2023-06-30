@echo off

set MYDIR=%~dp0
set FILENAME_PREFIX=mingw-w64-dpp

net session >nul 2>&1
if NOT %ERRORLEVEL% EQU 0 (
    echo ERROR: This script requires Administrator privileges!
    pause
    exit /b 1
)

where makecert.exe >nul 2>&1
IF %ERRORLEVEL% NEQ 0 (
    echo ERROR: makecert.exe not found, pleae add it to your PATH
    pause
    exit /b 1
)

where certmgr.exe >nul 2>&1
IF %ERRORLEVEL% NEQ 0 (
    echo ERROR: certmgr.exe not found, pleae add it to your PATH
    pause
    exit /b 1
)

where cert2spc.exe >nul 2>&1
IF %ERRORLEVEL% NEQ 0 (
    echo ERROR: cert2spc.exe not found, pleae add it to your PATH
    pause
    exit /b 1
)

where pvk2pfx.exe >nul 2>&1
IF %ERRORLEVEL% NEQ 0 (
    echo ERROR: pvk2pfx.exe not found, pleae add it to your PATH
    pause
    exit /b 1
)

makecert.exe -b 01/01/2023 -r -n CN="%FILENAME_PREFIX%" -sv "%MYDIR%/%FILENAME_PREFIX%.pvk" "%MYDIR%/%FILENAME_PREFIX%.cer"
certmgr.exe -add "%MYDIR%/%FILENAME_PREFIX%.cer" -s -r localMachine ROOT
certmgr.exe -add "%MYDIR%/%FILENAME_PREFIX%.cer" -s -r localMachine TRUSTEDPUBLISHER
cert2spc.exe "%MYDIR%/%FILENAME_PREFIX%.cer" "%MYDIR%/%FILENAME_PREFIX%.spc"
pvk2pfx.exe -pvk "%MYDIR%/%FILENAME_PREFIX%.pvk" -spc "%MYDIR%/%FILENAME_PREFIX%.spc" -pfx "%MYDIR%/%FILENAME_PREFIX%.pfx"

pause
