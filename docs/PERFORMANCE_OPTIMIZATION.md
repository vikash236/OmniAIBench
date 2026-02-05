# Hardware Monitor Optimization

## Performance Improvements

### Refresh Rate Optimization
To reduce CPU usage and prevent laptop overheating:

| Component | Old Interval | New Interval | Reduction |
|-----------|-------------|--------------|-----------|
| Dashboard | 2000ms | 5000ms | 60% fewer calls |
| Sensor Panel | 500ms | 2000ms | 75% fewer calls |
| Header | 2000ms | 5000ms | 60% fewer calls |

**Impact:**
- Reduced Python script executions from ~3/sec to ~0.6/sec
- Lower CPU usage
- Less heat generation
- Smoother performance

### Admin Elevation Setup

**Files configured:**
1. `src-tauri/tauri.manifest` - Windows UAC manifest
2. `src-tauri/build.rs` - Manifest embedding logic
3. `Cargo.toml` - Added `embed-resource` dependency

**Result:** App will now request admin privileges on launch, allowing:
- Full CPU temperature access via LibreHardwareMonitor
- GPU sensor access
- Complete hardware monitoring capabilities

## Testing
```bash
# Rebuild to apply manifest
cargo clean
npm run tauri build
```

The built executable will show UAC prompt on launch.
