@echo off
REM OmniAIBench Launcher - Auto-elevates to admin
REM This batch file launches PowerShell with the elevation script

set "SCRIPT_DIR=%~dp0"
set "APP_PATH=%SCRIPT_DIR%OmniAIBench.exe"

REM Check if already running as admin
net session >nul 2>&1
if %errorLevel% == 0 (
    REM Already admin - launch app directly
    start "" "%APP_PATH%"
) else (
    REM Not admin - request elevation
    powershell -NoProfile -ExecutionPolicy Bypass -Command "Start-Process '%APP_PATH%' -Verb RunAs"
)
