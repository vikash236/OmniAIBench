#!/usr/bin/env python3
"""
Comprehensive Sensor Monitoring
Detailed hardware metrics like HWiNFO
"""

import json
import platform
import subprocess
import sys

try:
    import cpuinfo
    import psutil
except ImportError:
    subprocess.check_call([sys.executable, "-m", "pip", "install", "py-cpuinfo", "psutil"])
    import cpuinfo
    import psutil


def get_detailed_cpu_sensors():
    """Get comprehensive CPU sensor data."""
    cpu_data = {
        "name": cpuinfo.get_cpu_info().get('brand_raw', 'Unknown CPU'),
        "cores": [],
        "package": {
            "temp": None,
            "power": None,
            "voltage": None
        }
    }
    
    # Per-core data
    core_count = psutil.cpu_count(logical=False)
    thread_count = psutil.cpu_count(logical=True)
    per_core_freq = psutil.cpu_freq(percpu=True) if hasattr(psutil.cpu_freq(percpu=True), '__iter__') else []
    per_core_load = psutil.cpu_percent(interval=0.01, percpu=True)
    
    for i in range(thread_count):
        core_info = {
            "id": i,
            "physical_core": i < core_count,
            "frequency": per_core_freq[i].current if i < len(per_core_freq) else 0,
            "load": per_core_load[i] if i < len(per_core_load) else 0,
            "temp": None  # Per-core temps need OpenHardwareMonitor
        }
        cpu_data["cores"].append(core_info)
    
    # Try to get package temp via WMI
    try:
        result = subprocess.run(
            ['powershell', '-Command', 
             'Get-WmiObject MSAcpi_ThermalZoneTemperature -Namespace root/wmi | Select-Object -First 1 CurrentTemperature'],
            capture_output=True,
            text=True,
            timeout=0.5
        )
        if result.returncode == 0 and 'CurrentTemperature' in result.stdout:
            lines = result.stdout.strip().split('\n')
            for line in lines:
                if line.strip().isdigit():
                    temp_kelvin = int(line.strip()) / 10.0
                    cpu_data["package"]["temp"] = round(temp_kelvin - 273.15, 1)
    except:
        pass
    
    return cpu_data


def get_detailed_gpu_sensors():
    """Get comprehensive GPU sensor data."""
    gpus = []
    
    # NVIDIA GPU (discrete)
    try:
        import pynvml
        pynvml.nvmlInit()
        device_count = pynvml.nvmlDeviceGetCount()
        
        for i in range(device_count):
            handle = pynvml.nvmlDeviceGetHandleByIndex(i)
            
            gpu_info = {
                "type": "discrete",
                "vendor": "NVIDIA",
                "name": pynvml.nvmlDeviceGetName(handle),
                "temp": pynvml.nvmlDeviceGetTemperature(handle, 0),
                "utilization": {
                    "gpu": pynvml.nvmlDeviceGetUtilizationRates(handle).gpu,
                    "memory": pynvml.nvmlDeviceGetUtilizationRates(handle).memory
                },
                "memory": {
                    "total": round(pynvml.nvmlDeviceGetMemoryInfo(handle).total / (1024**3), 2),
                    "used": round(pynvml.nvmlDeviceGetMemoryInfo(handle).used / (1024**3), 2),
                    "free": round(pynvml.nvmlDeviceGetMemoryInfo(handle).free / (1024**3), 2)
                },
                "clocks": {
                    "core": pynvml.nvmlDeviceGetClockInfo(handle, 0),  # Graphics clock
                    "memory": pynvml.nvmlDeviceGetClockInfo(handle, 2)  # Memory clock
                },
                "power": {
                    "draw": round(pynvml.nvmlDeviceGetPowerUsage(handle) / 1000, 1),  # Convert mW to W
                    "limit": round(pynvml.nvmlDeviceGetPowerManagementLimit(handle) / 1000, 1)
                },
                "fan_speed": pynvml.nvmlDeviceGetFanSpeed(handle) if hasattr(pynvml, 'nvmlDeviceGetFanSpeed') else None
            }
            gpus.append(gpu_info)
        
        pynvml.nvmlShutdown()
    except:
        pass
    
    # AMD iGPU detection (basic)
    if platform.system() == "Windows":
        try:
            result = subprocess.run(
                ['wmic', 'path', 'win32_VideoController', 'get', 'name', '/value'],
                capture_output=True,
                text=True,
                timeout=0.5
            )
            for line in result.stdout.split('\n'):
                if 'Name=' in line and 'Radeon' in line:
                    gpu_name = line.split('=')[1].strip()
                    if gpu_name and 'Microsoft' not in gpu_name:
                        gpus.append({
                            "type": "integrated",
                            "vendor": "AMD",
                            "name": gpu_name,
                            "temp": None,
                            "utilization": {"gpu": None},
                            "memory": None,
                            "clocks": None
                        })
                        break
        except:
            pass
    
    return gpus


def get_detailed_memory_sensors():
    """Get detailed RAM information."""
    mem = psutil.virtual_memory()
    
    return {
        "total_gb": round(mem.total / (1024**3), 2),
        "used_gb": round(mem.used / (1024**3), 2),
        "free_gb": round(mem.free / (1024**3), 2),
        "available_gb": round(mem.available / (1024**3), 2),
        "percent_used": mem.percent,
        "speed_mhz": None  # Requires WMI or dmidecode
    }


def get_storage_sensors():
    """Get storage device information."""
    storage = []
    
    partitions = psutil.disk_partitions()
    for partition in partitions:
        if 'cdrom' in partition.opts or partition.fstype == '':
            continue
        
        try:
            usage = psutil.disk_usage(partition.mountpoint)
            io_counters = psutil.disk_io_counters(perdisk=False)
            
            storage.append({
                "name": partition.device,
                "mountpoint": partition.mountpoint,
                "fstype": partition.fstype,
                "total_gb": round(usage.total / (1024**3), 2),
                "used_gb": round(usage.used / (1024**3), 2),
                "free_gb": round(usage.free / (1024**3), 2),
                "percent_used": usage.percent,
                "temp": None  # Requires SMART data
            })
        except:
            pass
    
    return storage


def get_network_sensors():
    """Get network statistics."""
    net_io = psutil.net_io_counters()
    
    return {
        "bytes_sent": net_io.bytes_sent,
        "bytes_recv": net_io.bytes_recv,
        "packets_sent": net_io.packets_sent,
        "packets_recv": net_io.packets_recv
    }


def get_battery_sensors():
    """Get battery information if available."""
    try:
        battery = psutil.sensors_battery()
        if battery:
            return {
                "percent": battery.percent,
                "plugged_in": battery.power_plugged,
                "time_left": battery.secsleft if battery.secsleft != psutil.POWER_TIME_UNLIMITED else None
            }
    except:
        pass
    
    return None


def get_all_sensors():
    """Get all detailed sensor data."""
    return {
        "cpu": get_detailed_cpu_sensors(),
        "gpus": get_detailed_gpu_sensors(),
        "memory": get_detailed_memory_sensors(),
        "storage": get_storage_sensors(),
        "network": get_network_sensors(),
        "battery": get_battery_sensors(),
        "npu": {
            "name": "AMD Ryzen AI",
            "tops": 10,
            "architecture": "Phoenix"
        } if '7840' in cpuinfo.get_cpu_info().get('brand_raw', '').lower() else None
    }


if __name__ == "__main__":
    sensors = get_all_sensors()
    print(json.dumps(sensors, indent=2))
