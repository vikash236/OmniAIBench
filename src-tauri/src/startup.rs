use std::process::Command;
use tauri::Manager;

// Auto-start OpenHardwareMonitor on app launch
#[tauri::command]
async fn initialize_monitoring() -> Result<(), String> {
    // Start OpenHardwareMonitor in background
    let _ = Command::new("python")
        .args(["../backend/ohm_manager.py"])
        .spawn();
    
    Ok(())
}

fn main() {
    tauri::Builder::default()
        .plugin(tauri_plugin_shell::init())
        .invoke_handler(tauri::generate_handler![
            scan_hardware,
            get_detailed_sensors,
            start_ohm,
            initialize_monitoring,
            get_required_environments,
            create_environments,
        ])
        .setup(|app| {
            // Auto-start OHM on app launch
            let window = app.get_window("main").unwrap();
            tauri::async_runtime::spawn(async move {
                let _ = initialize_monitoring().await;
            });
            Ok(())
        })
        .run(tauri::generate_context!())
        .expect("error while running tauri application");
}
