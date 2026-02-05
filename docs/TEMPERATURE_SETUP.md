# OpenHardwareMonitor Setup for CPU Temperature

To get CPU core temperatures like CPU-Z, you need to install OpenHardwareMonitor:

## Quick Setup (5 minutes)

### 1. Download OpenHardwareMonitor
- Go to: https://openhardwaremonitor.org/downloads/
- Download the latest version (0.9.6 or newer)
- Extract the ZIP file to `C:\Program Files\OpenHardwareMonitor\`

### 2. Run as Administrator
- Right-click `OpenHardwareMonitor.exe`
- Select "Run as administrator"
- **Important**: Keep it running in the background

### 3. Enable WMI Support
In OpenHardwareMonitor:
- Click `Options` → `Remote Web Server`
- Check "Run" 
- The app will now expose temperature data via WMI

### 4. Test in OmniAIBench
- Restart your OmniAIBench app
- CPU temperature should now show up!

---

## Current Status Without OpenHardwareMonitor

Your app currently shows:
- ✅ **CPU Load**: Real-time (updates every 0.5 seconds)
- ✅ **GPU Temp**: 45°C (NVIDIA detection working)
- ✅ **NPU**: 10 TOPS detected
- ⚠️ **CPU Temp**: N/A (needs OpenHardwareMonitor)

---

## Alternative: Use Without CPU Temperature

If you don't want to install OpenHardwareMonitor:
- Everything else works perfectly
- CPU temp will show "N/A"
- This is normal - Windows doesn't expose CPU temps by default
- The app is fully functional for benchmarking

---

## Why OpenHardwareMonitor?

Windows doesn't expose CPU core temperatures via standard APIs. Tools like CPU-Z, HWiNFO, and OpenHardwareMonitor use low-level hardware access to read temperature sensors directly. Our Python script can read this data via WMI once OpenHardwareMonitor exposes it.
