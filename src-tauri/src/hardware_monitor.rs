use std::process::{Command, Child, Stdio};
use std::sync::Mutex;
use tauri::Manager;

// Global handle to the hardware monitor process
static HARDWARE_MONITOR: Mutex<Option<Child>> = Mutex::new(None);

#[tauri::command]
pub async fn start_hardware_monitor() -> Result<(), String> {
    let exe_path = "../OmniHardwareMonitor/bin/Release/net8.0/win-x64/publish/OmniHardwareMonitor.exe";
    
    // Start the C# hardware monitor as a background process
    let child = Command::new(exe_path)
        .stdout(Stdio::piped())
        .stderr(Stdio::piped())
        .spawn()
        .map_err(|e| format!("Failed to start hardware monitor: {}", e))?;
    
    // Store the process handle
    let mut monitor = HARDWARE_MONITOR.lock().unwrap();
    *monitor = Some(child);
    
    Ok(())
}

#[tauri::command]
pub async fn get_hardware_sensors_stream() -> Result<String, String> {
    // Execute C# hardware monitor and transform to Dashboard format
    let exe_path = "../OmniHardwareMonitor/bin/Release/net8.0/win-x64/publish/OmniHardwareMonitor.exe";
    
    let output = Command::new(exe_path)
        .stdout(Stdio::piped())
        .stderr(Stdio::piped())
        .spawn()
        .map_err(|e| format!("Failed to start hardware monitor: {}", e))?
        .wait_with_output()
        .map_err(|e| format!("Failed to read output: {}", e))?;
    
    if output.status.success() {
        let result = String::from_utf8(output.stdout)
            .map_err(|e| format!("Invalid UTF-8 output: {}", e))?;
        
        // Get first line of JSON output (C# sensor array)
        let first_line = result.lines().next().unwrap_or("[]");
        
        // Transform C# format to Dashboard format
        crate::sensor_transformer::transform_sensors(first_line)
    } else {
        Err("Hardware monitor failed. Run as Administrator for full sensor access.".to_string())
    }
}

#[tauri::command]
pub async fn stop_hardware_monitor() -> Result<(), String> {
    let mut monitor = HARDWARE_MONITOR.lock().unwrap();
    
    if let Some(mut child) = monitor.take() {
        child.kill().map_err(|e| format!("Failed to stop monitor: {}", e))?;
    }
    
    Ok(())
}

pub fn init_hardware_monitor(app: &tauri::App) {
    // Auto-start hardware monitor when app launches
    tauri::async_runtime::spawn(async {
        let _ = start_hardware_monitor().await;
    });
}
