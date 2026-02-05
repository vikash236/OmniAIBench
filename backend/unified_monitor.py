#!/usr/bin/env python3
"""
Unified Hardware Monitor
Pure Python with LibreHardwareMonitor.dll integration
Fastest, most reliable sensor monitoring
"""

import json
import platform
import subprocess
import sys
import os

# Install required packages
try:
    import cpuinfo
    import psutil
except ImportError:
    subprocess.check_call([sys.executable, "-m", "pip", "install", "py-cpuinfo", "psutil"])
    import cpuinfo
    import psutil

# Try to import pythonnet for LibreHardwareMonitor
LIBREHW_AVAILABLE = False
try:
    import clr
    
    # Add LibreHardwareMonitor DLL path
    dll_path = os.path.join(os.path.dirname(__file__), "LibreHardwareMonitor")
    if os.path.exists(os.path.join(dll_path, "LibreHardwareMonitorLib.dll")):
        sys.path.append(dll_path)
        clr.AddReference("LibreHardwareMonitorLib")
        from LibreHardwareMonitor import Hardware
        LIBREHW_AVAILABLE = True
except:
    LIBREHW_AVAILABLE = False


class HardwareMonitor:
    """Unified hardware monitoring with LibreHardwareMonitor support."""
    
    def __init__(self):
        self.computer = None
        if LIBREHW_AVAILABLE:
            try:
                self.computer = Hardware.Computer()
                self.computer.IsCpuEnabled = True
                self.computer.IsGpuEnabled = True
                self.computer.IsMemoryEnabled = True
                self.computer.IsMotherboardEnabled = True
                self.computer.IsStorageEnabled = True
                self.computer.Open()
            except:
                self.computer = None
    
    def get_cpu_data(self):
        """Get comprehensive CPU data with per-core temps and frequencies."""
        cpu_data = {
            "name": cpuinfo.get_cpu_info().get('brand_raw', 'Unknown CPU'),
            "cores": [],
            "package": {"temp": None, "power": None, "voltage": None}
        }
        
        # Get per-core load and frequency
        core_count = psutil.cpu_count(logical=False)
        thread_count = psutil.cpu_count(logical=True)
        per_core_load = psutil.cpu_percent(interval=0.01, percpu=True)
        per_core_freq = psutil.cpu_freq(percpu=True) if hasattr(psutil, 'cpu_freq') else []
        
        # Get temps from LibreHardwareMonitor if available
        cpu_temps = {}
        if self.computer:
            try:
                self.computer.Hardware[0].Update()
                for hardware in self.computer.Hardware:
                    if str(hardware.HardwareType) == "Cpu":
                        hardware.Update()
                        for sensor in hardware.Sensors:
                            sensor_type = str(sensor.SensorType)
                            sensor_name = str(sensor.Name)
                            
                            if sensor_type == "Temperature":
                                if "Package" in sensor_name or "CPU" in sensor_name:
                                    cpu_data["package"]["temp"] = float(sensor.Value) if sensor.Value else None
                                elif "Core" in sensor_name:
                                    # Extract core number
                                    try:
                                        core_num = int(''.join(filter(str.isdigit, sensor_name)))
                                        cpu_temps[core_num] = float(sensor.Value) if sensor.Value else None
                                    except:
                                        pass
                            elif sensor_type == "Power" and "Package" in sensor_name:
                                cpu_data["package"]["power"] = float(sensor.Value) if sensor.Value else None
                            elif sensor_type == "Voltage" and "Core" in sensor_name:
                                cpu_data["package"]["voltage"] = float(sensor.Value) if sensor.Value else None
            except:
                pass
        
        # Build per-core data
        for i in range(thread_count):
            core_info = {
                "id": i,
                "physical_core": i < core_count,
                "frequency": per_core_freq[i].current if i < len(per_core_freq) else 0,
                "load": per_core_load[i] if i < len(per_core_load) else 0,
                "temp": cpu_temps.get(i)
            }
            cpu_data["cores"].append(core_info)
        
        return cpu_data
    
    def get_gpu_data(self):
        """Get detailed GPU data from NVIDIA or LibreHardwareMonitor."""
        gpus = []
        
        # Try NVIDIA first
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
                        "core": pynvml.nvmlDeviceGetClockInfo(handle, 0),
                        "memory": pynvml.nvmlDeviceGetClockInfo(handle, 2)
                    },
                    "power": {
                        "draw": round(pynvml.nvmlDeviceGetPowerUsage(handle) / 1000, 1),
                        "limit": round(pynvml.nvmlDeviceGetPowerManagementLimit(handle) / 1000, 1)
                    },
                    "fan_speed": pynvml.nvmlDeviceGetFanSpeed(handle) if hasattr(pynvml, 'nvmlDeviceGetFanSpeed') else None
                }
                gpus.append(gpu_info)
            
            pynvml.nvmlShutdown()
        except:
            pass
        
        # Detect AMD iGPU
        if platform.system() == "Windows":
            try:
                result = subprocess.run(
                    ['wmic', 'path', 'win32_VideoController', 'get', 'name', '/value'],
                    capture_output=True, text=True, timeout=0.5
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
                            })
                            break
            except:
                pass
        
        return gpus
    
    def get_memory_data(self):
        """Get RAM information."""
        mem = psutil.virtual_memory()
        return {
            "total_gb": round(mem.total / (1024**3), 2),
            "used_gb": round(mem.used / (1024**3), 2),
            "free_gb": round(mem.free / (1024**3), 2),
            "available_gb": round(mem.available / (1024**3), 2),
            "percent_used": mem.percent,
        }
    
    def get_storage_data(self):
        """Get storage information."""
        storage = []
        for partition in psutil.disk_partitions():
            if 'cdrom' in partition.opts or partition.fstype == '':
                continue
            try:
                usage = psutil.disk_usage(partition.mountpoint)
                storage.append({
                    "name": partition.device,
                    "mountpoint": partition.mountpoint,
                    "total_gb": round(usage.total / (1024**3), 2),
                    "used_gb": round(usage.used / (1024**3), 2),
                    "free_gb": round(usage.free / (1024**3), 2),
                    "percent_used": usage.percent,
                })
            except:
                pass
        return storage
    
    def get_network_data(self):
        """Get network statistics."""
        net_io = psutil.net_io_counters()
        return {
            "bytes_sent": net_io.bytes_sent,
            "bytes_recv": net_io.bytes_recv,
        }
    
    def get_battery_data(self):
        """Get battery information."""
        try:
            battery = psutil.sensors_battery()
            if battery:
                return {
                    "percent": battery.percent,
                    "plugged_in": battery.power_plugged,
                }
        except:
            pass
        return None
    
    def get_npu_data(self):
        """Detect NPU."""
        cpu_name = cpuinfo.get_cpu_info().get('brand_raw', '').lower()
        if '7840' in cpu_name or '7940' in cpu_name:
            return {"name": "AMD Ryzen AI", "tops": 10, "architecture": "Phoenix"}
        return None
    
    def get_all_sensors(self):
        """Get all sensor data."""
        return {
            "cpu": self.get_cpu_data(),
            "gpus": self.get_gpu_data(),
            "memory": self.get_memory_data(),
            "storage": self.get_storage_data(),
            "network": self.get_network_data(),
            "battery": self.get_battery_data(),
            "npu": self.get_npu_data(),
        }
    
    def close(self):
        """Cleanup."""
        if self.computer:
            try:
                self.computer.Close()
            except:
                pass


# Global monitor instance
_monitor = None

def get_monitor():
    """Get or create monitor instance."""
    global _monitor
    if _monitor is None:
        _monitor = HardwareMonitor()
    return _monitor


if __name__ == "__main__":
    monitor = get_monitor()
    sensors = monitor.get_all_sensors()
    print(json.dumps(sensors, indent=2))
