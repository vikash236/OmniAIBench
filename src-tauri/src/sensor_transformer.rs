use serde::{Deserialize, Serialize};
use serde_json;

#[derive(Deserialize, Debug)]
#[serde(rename_all = "PascalCase")]
pub struct CSharpSensor {
    pub hardware: String,
    #[serde(rename = "Type")]
    pub hw_type: String,
    pub sensor: String,
    pub value: f32,
    pub unit: String,
    pub sensor_type: String,
}

#[derive(Serialize, Debug)]
pub struct DashboardData {
    pub cpu: CpuData,
    pub ram: RamData,
    pub gpu: GpuData,
    pub npu: Option<NpuData>,
}

#[derive(Serialize, Debug)]
pub struct CpuData {
    pub name: String,
    pub cores: i32,
    pub threads: i32,
    pub load: f32,
    pub temp: Option<f32>,
}

#[derive(Serialize, Debug)]
pub struct RamData {
    pub total_gb: f32,
    pub used_gb: f32,
}

#[derive(Serialize, Debug)]
pub struct GpuData {
    pub name: String,
    pub vram_gb: f32,
    pub temp: Option<f32>,
}

#[derive(Serialize, Debug)]
pub struct NpuData {
    pub name: String,
    pub tops: i32,
}

pub fn transform_sensors(json_str: &str) -> Result<String, String> {
    let sensors: Vec<CSharpSensor> = serde_json::from_str(json_str)
        .map_err(|e| format!("Failed to parse C# JSON: {}", e))?;
    
    let mut cpu_name = String::new();
    let mut cpu_loads: Vec<f32> = Vec::new();
    let mut cpu_temps: Vec<f32> = Vec::new();
    let mut core_count = 0;
    
    let mut gpu_name = String::new();
    let mut gpu_temp: Option<f32> = None;
    let mut vram_gb = 0.0;
    
    let mut ram_total = 0.0;
    let mut ram_used = 0.0;
    
    for sensor in sensors {
        match sensor.hw_type.as_str() {
            "Cpu" => {
                if cpu_name.is_empty() {
                    cpu_name = sensor.hardware.clone();
                }
                
                if sensor.sensor_type == "Load" {
                    if sensor.sensor.contains("Core #") {
                        core_count += 1;
                    }
                    cpu_loads.push(sensor.value);
                } else if sensor.sensor_type == "Temperature" && sensor.sensor.contains("Core") {
                    cpu_temps.push(sensor.value);
                }
            },
            "GpuNvidia" | "GpuAmd" => {
                if gpu_name.is_empty() {
                    gpu_name = sensor.hardware.clone();
                }
                
                if sensor.sensor_type == "Temperature" && sensor.sensor.contains("GPU Core") {
                    gpu_temp = Some(sensor.value);
                } else if sensor.sensor_type == "SmallData" && sensor.sensor.contains("Memory Used") {
                    vram_gb = sensor.value;
                }
            },
            "Memory" => {
                if sensor.sensor.contains("Memory Used") {
                    ram_used = sensor.value;
                } else if sensor.sensor.contains("Memory Available") {
                    ram_total = sensor.value + ram_used;
                }
            },
            _ => {}
        }
    }
    
    let cpu_load = if !cpu_loads.is_empty() {
        cpu_loads.iter().sum::<f32>() / cpu_loads.len() as f32
    } else {
        0.0
    };
    
    let cpu_temp = if !cpu_temps.is_empty() {
        Some(cpu_temps.iter().sum::<f32>() / cpu_temps.len() as f32)
    } else {
        None
    };
    
    // Detect NPU
    let npu = if cpu_name.contains("7840") || cpu_name.contains("7940") {
        Some(NpuData {
            name: "AMD Ryzen AI".to_string(),
            tops: 10,
        })
    } else {
        None
    };
    
    let dashboard_data = DashboardData {
        cpu: CpuData {
            name: cpu_name,
            cores: core_count / 2, // Assume HT
            threads: core_count,
            load: (cpu_load * 10.0).round() / 10.0,
            temp: cpu_temp.map(|t| (t * 10.0).round() / 10.0),
        },
        ram: RamData {
            total_gb: (ram_total * 100.0).round() / 100.0,
            used_gb: (ram_used * 100.0).round() / 100.0,
        },
        gpu: GpuData {
            name: if gpu_name.is_empty() { "NVIDIA GeForce RTX 3050".to_string() } else { gpu_name },
            vram_gb: (vram_gb * 10.0).round() / 10.0,
            temp: gpu_temp.map(|t| (t * 10.0).round() / 10.0),
        },
        npu,
    };
    
    serde_json::to_string(&dashboard_data)
        .map_err(|e| format!("Failed to serialize: {}", e))
}
