@echo off
REM OmniAIBench Quick Setup Script for Windows
REM This script will check for and install all required dependencies

echo ========================================
echo  OmniAIBench - Quick Setup
echo ========================================
echo.

REM Check for Rust
echo [1/4] Checking for Rust...
where cargo >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo [!] Rust not found. Installing Rust via winget...
    winget install --id=Rustlang.Rustup -e
    echo [!] Please restart your terminal after Rust installation completes
    echo [!] Then run this script again
    pause
    exit /b 1
) else (
    echo [+] Rust is installed
    cargo --version
)

REM Check for Node.js
echo.
echo [2/4] Checking for Node.js...
where node >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo [!] Node.js not found. Please install from https://nodejs.org/
    pause
    exit /b 1
) else (
    echo [+] Node.js is installed
    node --version
)

REM Check for Python
echo.
echo [3/4] Checking for Python...
where python >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo [!] Python not found. Please install from https://python.org/
    pause
    exit /b 1
) else (
    echo [+] Python is installed
    python --version
)

REM Install dependencies
echo.
echo [4/4] Installing dependencies...
echo [+] Installing Node packages...
call npm install

echo [+] Installing Python packages...
python -m pip install -r backend\requirements.txt

echo.
echo ========================================
echo  Setup Complete!
echo ========================================
echo.
echo Next steps:
echo 1. Make sure you have .env file with Firebase/Supabase credentials
echo 2. Run: npm run tauri dev
echo.
pause
