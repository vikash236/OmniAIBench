# OmniAIBench Launcher with Auto-Admin Request
# This script checks if running as admin, if not, re-launches with elevation

param([string]$AppPath = "$PSScriptRoot\OmniAIBench.exe")

# Check if running as administrator
$isAdmin = ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)

if (-not $isAdmin) {
    # Not admin - relaunch with elevation
    try {
        Start-Process powershell.exe -ArgumentList "-NoProfile -ExecutionPolicy Bypass -File `"$PSCommandPath`" -AppPath `"$AppPath`"" -Verb RunAs
    } catch {
        # User cancelled UAC or error occurred
        $msg = "Administrator privileges are required for OmniAIBench to access hardware sensors.`n`nPlease click 'Yes' when prompted for admin access."
        [System.Windows.Forms.MessageBox]::Show($msg, "Admin Required", "OK", "Warning")
    }
    exit
}

# Running as admin - launch the app
if (Test-Path $AppPath) {
    Start-Process $AppPath -WorkingDirectory (Split-Path $AppPath)
} else {
    [System.Windows.Forms.MessageBox]::Show("Could not find OmniAIBench.exe", "Error", "OK", "Error")
}
