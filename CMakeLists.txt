cmake_minimum_required(VERSION 3.28)
project(OmniAIBench VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Qt 6 for GUI (NO REACT/TAURI - Native Qt)
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Charts)

# SQLite for local storage
find_package(unofficial-sqlite3 CONFIG REQUIRED)

# JSON library
find_package(nlohmann_json CONFIG REQUIRED)

# Source files
add_executable(OmniAIBench WIN32
    src/main.cpp
    src/core/hardware_detector.cpp
    src/core/hardware_detector.h
    src/core/sensor_monitor.cpp
    src/core/sensor_monitor.h
    src/ui/main_window.cpp
    src/ui/main_window.h
    src/ui/dashboard_widget.cpp
    src/ui/dashboard_widget.h
    src/ui/sensor_panel_widget.h
    src/ui/cpu_benchmark_widget.h
    src/ui/gpu_benchmark_widget.h
    src/ui/npu_benchmark_widget.h
    src/ui/leaderboard_widget.h
    src/utils/logger.cpp
    src/utils/logger.h
)

target_include_directories(OmniAIBench PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(OmniAIBench PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Charts
    unofficial::sqlite3::sqlite3
    nlohmann_json::nlohmann_json
    wbemuuid  # WMI library for hardware detection
)

# Windows-specific settings
if(WIN32)
    set_target_properties(OmniAIBench PROPERTIES
        VS_DPI_AWARE "PerMonitor"
    )
    
    # Embed admin manifest for UAC elevation (required for WMI thermal zones)
    if(MSVC)
        # For MSVC, use mt.exe to embed manifest after build
        add_custom_command(TARGET OmniAIBench POST_BUILD
            COMMAND mt.exe -nologo -manifest "${CMAKE_SOURCE_DIR}/src/OmniAIBench.manifest" 
                    -outputresource:"$<TARGET_FILE:OmniAIBench>"\;1
            COMMENT "Embedding admin manifest for UAC elevation..."
        )
    endif()
endif()

