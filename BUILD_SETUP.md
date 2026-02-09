# OmniAIBench C++ Build Setup Guide

## Step-by-Step Dependency Installation

### 1. Install vcpkg (if not already installed)

```powershell
# Clone vcpkg to C:\ drive
git clone https://github.com/Microsoft/vcpkg.git c:\vcpkg
cd c:\vcpkg

# Bootstrap vcpkg
.\bootstrap-vcpkg.bat

# Add to PATH (optional, but recommended)
[Environment]::SetEnvironmentVariable("Path", $env:Path + ";C:\vcpkg", [EnvironmentVariableTarget]::User)
```

### 2. Install All Dependencies

This will take ~30-60 minutes on first run as it compiles all libraries:

```powershell
cd c:\vcpkg

# Qt 6 (Native UI)
.\vcpkg install qt6-base:x64-windows
.\vcpkg install qt6-charts:x64-windows

# AI Frameworks
.\vcpkg install onnxruntime-gpu:x64-windows
.\vcpkg install openvino:x64-windows

# Graphics APIs
.\vcpkg install vulkan:x64-windows
.\vcpkg install directx-headers:x64-windows

# Utilities
.\vcpkg install sqlite3:x64-windows
.\vcpkg install nlohmann-json:x64-windows

# NVIDIA NVML (if you have NVIDIA GPU)
# Download CUDA Toolkit: https://developer.nvidia.com/cuda-downloads
```

### 3. Download Embedded Python

```powershell
cd c:\Projects\OmniAIBench

# Download Python 3.11 embeddable
Invoke-WebRequest -Uri "https://www.python.org/ftp/python/3.11.8/python-3.11.8-embed-amd64.zip" -OutFile "python-embed.zip"

# Extract
Expand-Archive python-embed.zip -DestinationPath "python_embedded"
Remove-Item python-embed.zip

# Install pip
Invoke-WebRequest -Uri "https://bootstrap.pypa.io/get-pip.py" -OutFile "python_embedded\get-pip.py"
.\python_embedded\python.exe python_embedded\get-pip.py

# Install dependencies
.\python_embedded\python.exe -m pip install huggingface-hub tqdm
```

### 4. Build OmniAIBench

```powershell
cd c:\Projects\OmniAIBench

# Configure with CMake
cmake -B build -S . `
  -DCMAKE_TOOLCHAIN_FILE=c:/vcpkg/scripts/buildsystems/vcpkg.cmake `
  -DCMAKE_BUILD_TYPE=Release

# Build (this will take a few minutes)
cmake --build build --config Release

# Check output
ls build\Release\OmniAIBench.exe
```

### 5. Run the Application

```powershell
cd build\Release
.\OmniAIBench.exe
```

## Troubleshooting

### Qt Not Found
```
Error: Could not find Qt6
```
**Solution**: Make sure vcpkg installed Qt6 successfully:
```powershell
c:\vcpkg\vcpkg list | Select-String "qt6"
```

### OpenVINO Not Found
**Solution**: Install OpenVINO via vcpkg:
```powershell
c:\vcpkg\vcpkg install openvino:x64-windows
```

### Build Errors
If you get build errors, try cleaning and rebuilding:
```powershell
Remove-Item -Recurse -Force build
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=c:/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Release
```

## Next Steps

Once built successfully:
1. See `driver_signing_guide.md` for kernel driver setup
2. Run the application - AI models will download automatically on first run
3. Explore the GUI and run benchmarks!

## Total Disk Space Required

- vcpkg + dependencies: ~5-10 GB
- Python embedded: ~10 MB
- AI models (downloaded at runtime): ~500 MB
- Build output: ~50-100 MB

**Total**: ~5-6 GB
