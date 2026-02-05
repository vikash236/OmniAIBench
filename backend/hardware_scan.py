#!/usr/bin/env python3
"""
Hardware Scanner with OpenHardwareMonitor Support
Real-time CPU core temperatures like CPU-Z
"""

import json
import platform
import subprocess
import sys
import os

try:
    import cpuinfo
    import psutil
except ImportError:
    subprocess.check_call([sys.executable, "-m", "pip", "install", "py-cpuinfo", "psutil"])
    import cpuinfo
    import psutil


def get_cpu_temp_openhardwaremonitor():
    """Get CPU temperature using OpenHardwareMonitor via WMI."""
    try:
        # Try reading from OpenHardwareMonitor's WMI interface
        result = subprocess.run(
            ['powershell', '-Command', 
             'Get-WmiObject -Namespace root/OpenHardwareMonitor -Class Sensor | Where-Object {$_.SensorType -eq "Temperature" -and $_.Name -like "*CPU Package*"} | Select-Object -First 1 -ExpandProperty Value'],
            capture_output=True,
            text=True,
            timeout=1
        )
        
        if result.returncode == 0 and result.stdout.strip():
            try:
                temp = float(result.stdout.strip())
                return round(temp, 1)
            except ValueError:
                pass
    except Exception:
        pass
    
    return None


def get_cpu_temp_windows_wmi():
    """Get CPU temperature using Windows WMI thermal zone."""
    try:
        result = subprocess.run(
            ['powershell', '-Command', 
             'Get-WmiObject MSAcpi_ThermalZoneTemperature -Namespace root/wmi | Select-Object -First 1 CurrentTemperature'],
            capture_output=True,
            text=True,
            timeout=1
        )
        
        if result.returncode == 0 and 'CurrentTemperature' in result.stdout:
            lines = result.stdout.strip().split('\n')
            for line in lines:
                if line.strip().isdigit():
                    temp_kelvin = int(line.strip()) / 10.0
                    temp_celsius = temp_kelvin - 273.15
                    return round(temp_celsius, 1)
    except Exception:
        pass
    
    return None


def get_cpu_info():
    """Get detailed CPU information with temperature monitoring."""
    cpu_info = {
        "name": cpuinfo.get_cpu_info().get('brand_raw', 'Unknown CPU'),
        "cores": psutil.cpu_count(logical=False),
        "threads": psutil.cpu_count(logical=True),
        "max_freq_mhz": psutil.cpu_freq().max if psutil.cpu_freq() else 0,
        "temp": None,
        "load": None
    }
    
    # Try multiple temperature sources
    if platform.system() == "Windows":
        # Try OpenHardwareMonitor first (most reliable)
        temp = get_cpu_temp_openhardwaremonitor()
        if temp is not None:
            cpu_info["temp"] = temp
        else:
            # Fallback to WMI thermal zone
            temp = get_cpu_temp_windows_wmi()
            if temp is not None:
                cpu_info["temp"] = temp
    elif hasattr(psutil, "sensors_temperatures"):
        # Linux/macOS temperature sensors
        temps = psutil.sensors_temperatures()
        if temps:
            for name, entries in temps.items():
                if entries:
                    cpu_info["temp"] = round(entries[0].current, 1)
                    break
    
    # Get CPU load (very fast for 0.5s updates)
    try:
        cpu_info["load"] = round(psutil.cpu_percent(interval=0.01), 1)
    except Exception:
        cpu_info["load"] = 0.0
    
    return cpu_info


def get_ram_info():
    """Get RAM information"""
    mem = psutil.virtual_memory()
    
    return {
        "total_gb": round(mem.total / (1024 ** 3), 2),
        "used_gb": round(mem.used / (1024 ** 3), 2),
        "speed_mhz": None,
    }


def get_gpu_info():
    """Get GPU information with temperature."""
    gpu_info = {
        "name": None,
        "vram_gb": None,
        "temp": None,
    }
    
    # Try NVIDIA
    try:
        import pynvml
        pynvml.nvmlInit()
        handle = pynvml.nvmlDeviceGetHandleByIndex(0)
        gpu_info["name"] = pynvml.nvmlDeviceGetName(handle)
        mem_info = pynvml.nvmlDeviceGetMemoryInfo(handle)
        gpu_info["vram_gb"] = round(mem_info.total / (1024 ** 3), 2)
        gpu_info["temp"] = pynvml.nvmlDeviceGetTemperature(handle, 0)
        pynvml.nvmlShutdown()
        return gpu_info
    except:
        pass
    
    # Fallback to Windows WMI
    if platform.system() == "Windows":
        try:
            result = subprocess.run(
                ['wmic', 'path', 'win32_VideoController', 'get', 'name', '/value'],
                capture_output=True,
                text=True,
                timeout=1
            )
            for line in result.stdout.split('\n'):
                if 'Name=' in line:
                    gpu_name = line.split('=')[1].strip()
                    if gpu_name and 'Microsoft' not in gpu_name:
                        gpu_info["name"] = gpu_name
                        break
        except:
            pass
    
    return gpu_info


def detect_amd_ryzen_ai_npu():
    """Detect AMD Ryzen AI NPU."""
    try:
        cpu_name = cpuinfo.get_cpu_info().get('brand_raw', '').lower()
        
        npu_configs = {
            '7840': {'name': 'AMD Ryzen AI', 'tops': 10, 'arch': 'Phoenix'},
            '7940': {'name': 'AMD Ryzen AI', 'tops': 10, 'arch': 'Phoenix'},
            '7640': {'name': 'AMD Ryzen AI', 'tops': 10, 'arch': 'Phoenix'},
            '7740': {'name': 'AMD Ryzen AI', 'tops': 10, 'arch': 'Phoenix'},
            '8840': {'name': 'AMD Ryzen AI', 'tops': 16, 'arch': 'Hawk Point'},
            '8940': {'name': 'AMD Ryzen AI', 'tops': 16, 'arch': 'Hawk Point'},
            '8640': {'name': 'AMD Ryzen AI', 'tops': 16, 'arch': 'Hawk Point'},
            '8740': {'name': 'AMD Ryzen AI', 'tops': 16, 'arch': 'Hawk Point'},
            '365': {'name': 'AMD Ryzen AI 300', 'tops': 50, 'arch': 'Strix Point'},
            '375': {'name': 'AMD Ryzen AI 300', 'tops': 50, 'arch': 'Strix Point'},
        }
        
        for model_id, npu_info in npu_configs.items():
            if model_id in cpu_name:
                return {
                    'name': npu_info['name'],
                    'tops': npu_info['tops'],
                    'architecture': npu_info['arch']
                }
        
        return {'name': None, 'tops': None, 'architecture': None}
    except Exception:
        return {'name': None, 'tops': None, 'architecture': None}


def scan_hardware():
    """Main hardware scan function."""
    return {
        "cpu": get_cpu_info(),
        "ram": get_ram_info(),
        "gpu": get_gpu_info(),
        "npu": detect_amd_ryzen_ai_npu(),
        "os": platform.system(),
    }


if __name__ == "__main__":
    hardware_info = scan_hardware()
    print(json.dumps(hardware_info, indent=2))
