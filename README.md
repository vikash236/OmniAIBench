# OmniAIBench 🚀

A comprehensive hardware benchmarking and monitoring application with professional VS Code-inspired UI.

![Version](https://img.shields.io/badge/version-1.0.0-blue)
![Platform](https://img.shields.io/badge/platform-Windows-blue)
![License](https://img.shields.io/badge/license-MIT-green)

## ✨ Features

### 📊 Real-Time Hardware Monitoring
- **CPU Monitoring**: Core temperatures, frequencies, loads (per-thread)
- **GPU Monitoring**: Temperature, utilization, VRAM, power draw, fan speeds
- **RAM Monitoring**: Usage, available memory
- **NPU Detection**: AMD Ryzen AI support
- **Comprehensive Sensors**: Voltages, power consumption, fan speeds

### 🏆 Benchmark Suite
- **CPU Benchmark**: Single-core and multi-core performance tests
- **GPU Benchmark**: Graphics + compute tests with live FPS tracking
- **NPU Benchmark**: AI inference performance testing
- **Score Tracking**: Local leaderboard with export to JSON

### 🎨 Professional UI
- Clean VS Code Dark+ theme
- Flat minimal design
- Color-coded components (Blue=CPU, Amber=GPU, Yellow=NPU, Green=RAM)
- Responsive layout
- Small, clean fonts (13px base)

## 📦 Installation

### Prerequisites
- Windows 10/11 (x64)
- .NET 8.0 Runtime (bundled with installer)
- WebView2 Runtime (auto-installed)

### Install Steps

1. **Download installer** from Releases:
   - `OmniAIBench_1.0.0_x64_en-US.msi` (recommended)
   - `OmniAIBench_1.0.0_x64-setup.exe` (NSIS)

2. **Run installer** and follow wizard

3. **Launch with admin privileges**:
   ```
   Navigate to install directory
   Run: OmniAIBench-Launcher.bat
   Click "Yes" on UAC prompt
   ```

## 🔧 Building from Source

### Requirements
- Node.js 18+
- Rust 1.70+
- .NET 8.0 SDK
- Python 3.8+ (optional, for backend monitoring)

### Build Steps

```bash
# Clone repository
git clone https://github.com/vikash236/OmniAIBench.git
cd OmniAIBench

# Install dependencies
npm install

# Build C# sidecar
cd OmniHardwareMonitor
dotnet publish -c Release -r win-x64 --self-contained
cd ..

# Development mode
npm run tauri dev

# Production build
npm run tauri build
```

**Output**: Installers in `src-tauri/target/release/bundle/`

## ⚡ Auto-Admin Elevation

OmniAIBench requires administrator privileges for full hardware sensor access.

**Solution**: Use included `OmniAIBench-Launcher.bat`

### How it works:
1. Launcher checks if running as admin
2. Shows UAC prompt if needed
3. Launches app with admin rights
4. Full sensor access enabled!

### Create Desktop Shortcut:
1. Open install directory
2. Right-click `OmniAIBench-Launcher.bat`
3. Send to → Desktop (create shortcut)

## 🎯 Usage

### Dashboard
- View real-time hardware stats
- CPU/GPU/RAM overview
- Auto-refresh every 5 seconds

### Sensor Panel
- Click "Sensors" button in header
- Categorized sensor view (temperatures, utilization, frequencies, power, fans, memory)

### Benchmarks
- Navigate to CPU/GPU/NPU Benchmark pages
- Click "Start Benchmark"
- View progress and results
- Scores automatically saved to leaderboard

### Leaderboard
- View all test results
- Filter by test type
- Export to JSON

## 🛠️ Tech Stack

**Frontend:** React 18, TypeScript, Vite, Tailwind CSS
**Backend:** Tauri v2 (Rust), C# LibreHardwareMonitor, Python
**Monitoring:** LibreHardwareMonitor, psutil, pynvml, Windows WMI

## 📁 Project Structure

```
OmniAIBench/
├── src/                      # React frontend
│   ├── pages/               # App pages (Dashboard, Benchmarks, etc.)
│   ├── components/          # Reusable components
│   └── styles/              # CSS styles
├── src-tauri/               # Tauri Rust backend
│   ├── OmniAIBench-Launcher.bat  # Auto-admin launcher
│   └── tauri.conf.json
├── OmniHardwareMonitor/     # C# sidecar
└── backend/                 # Python monitoring scripts
```

## 🐛 Troubleshooting

**No sensor data**: Ensure app runs as administrator via launcher.bat
**C# sidecar not starting**: Verify .NET 8.0 runtime installed
**UAC prompt missing**: Run launcher.bat, not .exe directly

## 🚀 Roadmap

- Real CPU stress tests (Web Workers)
- WebGL/WebGPU actual rendering benchmarks
- Cloud leaderboard (global rankings)
- Theme toggle (dark/light)
- Real-time performance graphs
- System comparison tools

## 📝 License

MIT License

## 👥 Contributors

- [vikash236](https://github.com/vikash236)

---

**Made with ❤️ for hardware enthusiasts**
