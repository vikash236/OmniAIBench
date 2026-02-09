# OmniAIBench - Professional Hardware & AI Benchmark Suite

A high-performance, native C++ benchmarking application for Windows using Qt 6, OpenVINO, and DirectX 12/Vulkan.

## 🆓 100% Free & Open Source

All technologies used are completely **free and open source** (MIT, Apache 2.0, LGPL licenses).

## Features

### 📊 Real-Time Hardware Monitoring
- **CPU**: Core temperatures, frequencies, per-thread loads, power consumption
- **GPU**: Temperature, utilization, VRAM, power draw, fan speeds
- **RAM**: Usage, available memory, frequency
- **NPU**: AMD Ryzen AI detection and monitoring
- **Comprehensive Sensors**: Voltages, fan speeds, NVMe temperatures

### 🏆 Benchmark Suite
- **CPU Benchmark**: Single-core + multi-core performance tests
- **GPU Benchmark**: Graphics + compute with live FPS tracking
- **NPU Benchmark**: AI inference testing (ResNet50, MobileNet, BERT)
- **Score Tracking**: Local leaderboard with JSON export

### 🎨 Professional UI
- VS Code Dark+ theme
- Color-coded components (Blue=CPU, Amber=GPU, Yellow=NPU, Green=RAM)
- Native Qt 6 interface (NO web technologies)

## Technology Stack

- **C++20** with MSVC 2022
- **Qt 6.6** (Native Windows UI)
- **ONNX Runtime** + **OpenVINO** (AI inference)
- **DirectX 12** + **Vulkan** (GPU benchmarks)
- **Custom kernel driver** (HWiNFO64-style sensor monitoring)
- **Python 3.11** (Model management)

## Build Instructions

### Prerequisites

1. **Visual Studio 2022** (Community Edition - FREE)
   - Install with "Desktop development with C++" workload
   - Include Windows 11 SDK

2. **vcpkg** (Package Manager)
   ```powershell
   git clone https://github.com/Microsoft/vcpkg.git c:\vcpkg
   cd c:\vcpkg
   .\bootstrap-vcpkg.bat
   ```

3. **Dependencies** (via vcpkg)
   ```powershell
   cd c:\vcpkg
   .\vcpkg install qt6-base:x64-windows qt6-charts:x64-windows
   .\vcpkg install onnxruntime-gpu:x64-windows 
   .\vcpkg install openvino:x64-windows
   .\vcpkg install sqlite3:x64-windows
   .\vcpkg install vulkan:x64-windows
   .\vcpkg install nlohmann-json:x64-windows
   ```

### Build Steps

```powershell
# Navigate to project
cd c:\Projects\OmniAIBench

# Configure CMake
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=c:/vcpkg/scripts/buildsystems/vcpkg.cmake

# Build
cmake --build build --config Release

# Output: build/Release/OmniAIBench.exe
```

## Driver Signing (for sensor monitoring)

See [`driver_signing_guide.md`](docs/driver_signing_guide.md) for detailed instructions on creating self-signed test certificates.

## First Run

On first run, the application will download AI models (~500 MB) from Hugging Face automatically.

## License

MIT License - See [LICENSE.md](LICENSE.md)

## Contributing

Contributions welcome! This is a 100% open-source project.

## Architecture

Based on professional benchmarking tools:
- **Geekbench AI Pro** architecture for AI benchmarks
- **HWiNFO64** kernel driver approach for sensors
- **3DMark** DirectX 12/Vulkan GPU testing

---

**Initial installer size**: ~15-20 MB  
**After model download**: ~500 MB total
