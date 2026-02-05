# Automatic Temperature Monitoring

## How It Works

OmniAIBench now **automatically manages OpenHardwareMonitor** for you!

### On First Launch
1. The app detects OpenHardwareMonitor is not installed
2. Downloads it automatically (~1MB)
3. Starts it in the background (minimized)
4. CPU core temperatures become available in the Sensors panel!

### After First Launch
- OpenHardwareMonitor runs silently in the background
- Provides real-time CPU core temperatures
- No manual setup required!

## What You'll See

**In the Sensors Panel:**
- ✅ Per-core CPU temperatures (all 8 cores)
- ✅ GPU temperature (already working)
- ✅ All other sensors

## Manual Control

If you want to manage OpenHardwareMonitor yourself:

**Start:**
```bash
python backend/ohm_manager.py
```

**Stop:**
```bash
python backend/ohm_manager.py --stop
```

## Technical Details

- **Download**: OpenHardwareMonitor v0.9.6 from official source
- **Location**: `backend/openhardwaremonitor/`
- **Process**: Runs as background process (no GUI shown)
- **Integration**: App reads temp via WMI interface

## Troubleshooting

**If temps still show "null":**
1. Check if OHM is running: `tasklist | findstr OpenHardwareMonitor`
2. Restart the app
3. Run OHM manually with admin rights once

**Administrator Rights:**
For full sensor access, OHM needs admin rights. The app will request elevation if needed.
