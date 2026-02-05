#!/usr/bin/env python3
"""
C# Sidecar Bridge
Parses OmniHardwareMonitor JSON and converts to our format
"""

import json
import sys

def parse_csharp_sensors(csharp_json):
    """Convert C# sensor array to our format."""
    sensors = json.loads(csharp_json)
    
    result = {
        "cpu": {"name": "", "cores": [], "load": 0.0, "temp": None},
        "ram": {"total_gb": 0, "used_gb": 0},
        "gpu": {"name": "", "vram_gb": 0, "temp": None},
        "npu": {"name": "AMD Ryzen AI", "tops": 10, "architecture": "Phoenix"},
        "os": "Windows"
    }
    
    # Parse CPU data
    cpu_temps = []
    cpu_loads = []
    for sensor in sensors:
        if sensor["Type"] == "Cpu":
            if not result["cpu"]["name"]:
                result["cpu"]["name"] = sensor["Hardware"]
            
            if "Core" in sensor["Sensor"] and sensor["SensorType"] == "Temperature":
                cpu_temps.append(sensor["Value"])
            elif "Load" in sensor["Sensor"] and sensor["SensorType"] == "Load":
                cpu_loads.append(sensor["Value"])
    
    if cpu_temps:
        result["cpu"]["temp"] = round(sum(cpu_temps) / len(cpu_temps), 1)
    if cpu_loads:
        result["cpu"]["load"] = round(sum(cpu_loads) / len(cpu_loads), 1)
    
    # Parse GPU data
    for sensor in sensors:
        if "GpuNvidia" in sensor["Type"] or "GpuAmd" in sensor["Type"]:
            if not result["gpu"]["name"]:
                result["gpu"]["name"] = sensor["Hardware"]
            
            if sensor["Sensor"] == "GPU Core" and sensor["SensorType"] == "Temperature":
                result["gpu"]["temp"] = round(sensor["Value"], 1)
            elif "Memory" in sensor["Sensor"] and sensor["SensorType"] == "Data":
                result["gpu"]["vram_gb"] = round(sensor["Value"], 1)
    
    # Parse RAM
    for sensor in sensors:
        if sensor["Type"] == "Memory":
            if "Used" in sensor["Sensor"]:
                result["ram"]["used_gb"] = round(sensor["Value"], 2)
            elif "Available" in sensor["Sensor"]:
                result["ram"]["total_gb"] = round(sensor["Value"], 2)
    
    return result

if __name__ == "__main__":
    # Read from stdin (C# output)
    csharp_output = sys.stdin.read()
    parsed = parse_csharp_sensors(csharp_output)
    print(json.dumps(parsed))
