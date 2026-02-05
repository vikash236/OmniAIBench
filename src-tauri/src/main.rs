// Prevents additional console window on Windows in release
#![cfg_attr(not(debug_assertions), windows_subsystem = "windows")]

mod hardware_monitor;
mod sensor_transformer;

use std::process::Command;
use serde::{Deserialize, Serialize};

#[cfg(windows)]
use std::os::windows::process::CommandExt;
use tauri::Emitter;

#[derive(Debug, Serialize, Deserialize)]
struct HardwareInfo {
    cpu: CPUInfo,
    ram: RAMInfo,
    gpu: GPUInfo,
    npu: NPUInfo,
}

#[derive(Debug, Serialize, Deserialize)]
struct CPUInfo {
    name: String,
    cores: u32,
    threads: u32,
    max_freq_mhz: f64,
}

#[derive(Debug, Serialize, Deserialize)]
struct RAMInfo {
    total_gb: f64,
    speed_mhz: Option<u32>,
}

#[derive(Debug, Serialize, Deserialize)]
struct GPUInfo {
    name: Option<String>,
    vram_gb: Option<f64>,
}

#[derive(Debug, Serialize, Deserialize)]
struct NPUInfo {
    name: Option<String>,
    tops: Option<f64>,
}

#[tauri::command]
async fn scan_hardware(app: tauri::AppHandle) -> Result<String, String> {
    // Try multiple possible locations for the script
    let exe_dir = std::env::current_exe()
        .ok()
        .and_then(|p| p.parent().map(|p| p.to_path_buf()));
    
    let mut tried_paths = Vec::new();
    let possible_paths = vec![
        // Dev path
        std::env::current_dir().ok().map(|p| p.join("backend").join("simple_scan.py")),
        // Next to exe
        exe_dir.as_ref().map(|p| p.join("backend").join("simple_scan.py")),
        // In resources folder (MSI install)
        exe_dir.as_ref().map(|p| p.join("resources").join("backend").join("simple_scan.py")),
        // In _up_ folder (some Tauri installs)
        exe_dir.as_ref().map(|p| p.join("_up_").join("backend").join("simple_scan.py")),
    ];
    
    let script_path = possible_paths.into_iter()
        .flatten()
        .inspect(|p| tried_paths.push(p.display().to_string()))
        .find(|p| p.exists())
        .ok_or_else(|| {
            format!("Script not found. Tried paths:\n{}", tried_paths.join("\n"))
        })?;
    
    let script_str = script_path.to_str()
        .ok_or("Invalid script path")?;
    
    
    #[cfg(windows)]
    let output = Command::new("python")
        .arg(script_str)
        .creation_flags(0x08000000) // CREATE_NO_WINDOW
        .output();
    
    #[cfg(not(windows))]
    let output = Command::new("python")
        .arg(script_str)
        .output();
    
    let output = output
        .map_err(|e| format!("Failed to execute hardware scan: {}", e))?;

    if output.status.success() {
        let result = String::from_utf8(output.stdout)
            .map_err(|e| format!("Invalid UTF-8 output: {}", e))?;
        Ok(result)
    } else {
        let error = String::from_utf8_lossy(&output.stderr);
        Err(format!("Hardware scan failed at path {:?}. Error: {}. Make sure Python is installed and in PATH.", script_path, error))
    }
}

#[tauri::command]
async fn get_detailed_sensors(app: tauri::AppHandle) -> Result<String, String> {
    // Try multiple possible locations
    let exe_dir = std::env::current_exe()
        .ok()
        .and_then(|p| p.parent().map(|p| p.to_path_buf()));
    
    let possible_paths = vec![
        std::env::current_dir().ok().map(|p| p.join("backend").join("unified_monitor.py")),
        exe_dir.as_ref().map(|p| p.join("backend").join("unified_monitor.py")),
        exe_dir.as_ref().map(|p| p.join("resources").join("backend").join("unified_monitor.py")),
        exe_dir.as_ref().map(|p| p.join("_up_").join("backend").join("unified_monitor.py")),
    ];
    
    let script_path = possible_paths.into_iter()
        .flatten()
        .find(|p| p.exists())
        .ok_or_else(|| "Script not found in any location".to_string())?;
    
    let script_str = script_path.to_str()
        .ok_or("Invalid script path")?;
    
    
    #[cfg(windows)]
    let output = Command::new("python")
        .arg(script_str)
        .creation_flags(0x08000000) // CREATE_NO_WINDOW
        .output();
    
    #[cfg(not(windows))]
    let output = Command::new("python")
        .arg(script_str)
        .output();
    
    let output = output
        .map_err(|e| format!("Failed to execute sensor monitor: {}", e))?;

    if output.status.success() {
        let result = String::from_utf8(output.stdout)
            .map_err(|e| format!("Invalid UTF-8 output: {}", e))?;
        Ok(result)
    } else {
        let error = String::from_utf8_lossy(&output.stderr);
        Err(format!("Sensor monitoring failed: {}", error))
    }
}

#[tauri::command]
async fn start_ohm() -> Result<(), String> {
    // Start OpenHardwareMonitor in background
    let output = Command::new("python")
        .args(["../backend/ohm_manager.py"])
        .output()
        .map_err(|e| format!("Failed to start OHM: {}", e))?;

    if output.status.success() {
        Ok(())
    } else {
        Err("Failed to start OpenHardwareMonitor".to_string())
    }
}

#[tauri::command]
async fn get_required_environments(hardware: String) -> Result<String, String> {
    // Execute Python environment manager to determine required environments
    let output = Command::new("python")
        .args(["backend/env_manager.py", "--get-required", &hardware])
        .output()
        .map_err(|e| format!("Failed to get environments: {}", e))?;

    if output.status.success() {
        let result = String::from_utf8(output.stdout)
            .map_err(|e| format!("Invalid UTF-8 output: {}", e))?;
        Ok(result)
    } else {
        Err("Failed to determine required environments".to_string())
    }
}

#[tauri::command]
async fn create_environments(
    env_names: Vec<String>,
    window: tauri::Window,
) -> Result<(), String> {
    for env_name in env_names {
        // Stream logs to frontend
        window.emit("env-log", format!("Creating environment: {}", env_name)).ok();
        
        let output = Command::new("python")
            .args(["backend/env_manager.py", "--create", &env_name])
            .output()
            .map_err(|e| format!("Failed to create environment {}: {}", env_name, e))?;
        
        if !output.status.success() {
            return Err(format!("Failed to create environment: {}", env_name));
        }
        
        window.emit("env-log", format!("✓ Environment {} created successfully", env_name)).ok();
    }
    
    Ok(())
}

fn main() {
    tauri::Builder::default()
        .plugin(tauri_plugin_shell::init())
        .invoke_handler(tauri::generate_handler![
            scan_hardware,
            get_detailed_sensors,
            hardware_monitor::start_hardware_monitor,
            hardware_monitor::get_hardware_sensors_stream,
            hardware_monitor::stop_hardware_monitor,
            start_ohm,
            get_required_environments,
            create_environments,
        ])
        .setup(|app| {
            // Auto-start hardware monitor on app launch
            hardware_monitor::init_hardware_monitor(app);
            Ok(())
        })
        .run(tauri::generate_context!())
        .expect("error while running tauri application");
}
