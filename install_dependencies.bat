@echo off
echo Installing dependencies via Visual Studio vcpkg...

set VCPKG="C:\Program Files\Microsoft Visual Studio\18\Community\VC\vcpkg\vcpkg.exe"

echo Integrating vcpkg with Visual Studio...
%VCPKG% integrate install

echo.
echo Installing Qt6 Base...
%VCPKG% install qt6-base:x64-windows

echo.
echo Installing Qt6 Charts...
%VCPKG% install qt6-charts:x64-windows

echo.
echo Installing SQLite3...
%VCPKG% install sqlite3:x64-windows

echo.
echo Installing Vulkan...
%VCPKG% install vulkan:x64-windows

echo.
echo Installing DirectX Headers...
%VCPKG% install directx-headers:x64-windows

echo.
echo Installing nlohmann-json...
%VCPKG% install nlohmann-json:x64-windows

echo.
echo NOTE: ONNX Runtime and OpenVINO will be installed manually if not available in vcpkg
echo.
echo Attempting ONNX Runtime installation...
%VCPKG% search onnxruntime
%VCPKG% install onnxruntime-gpu:x64-windows

echo.
echo Attempting OpenVINO installation...
%VCPKG% search openvino  
%VCPKG% install openvino:x64-windows

echo.
echo ========================================
echo Dependency installation complete!
echo ========================================
pause
