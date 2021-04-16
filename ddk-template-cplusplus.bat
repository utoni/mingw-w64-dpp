@echo off
set SERVICE_NAME=DDK-Template-CPlusPlus
set DRIVER="%~dp0\ddk-template-cplusplus.sys"

net session >nul 2>&1
if NOT %ERRORLEVEL% EQU 0 (
    echo ERROR: This script requires Administrator privileges!
    pause
    exit /b 1
)

echo ---------------------------------------
echo -- Service Name: %SERVICE_NAME%
echo -- Driver......: %DRIVER%
echo ---------------------------------------

sc create %SERVICE_NAME% binPath= %DRIVER% type= kernel
echo ---------------------------------------
sc start %SERVICE_NAME%
echo ---------------------------------------
sc query %SERVICE_NAME%
echo [PRESS A KEY TO STOP THE DRIVER]
pause
sc stop %SERVICE_NAME%
sc delete %SERVICE_NAME%
echo Done.
timeout /t 3
