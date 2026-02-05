#!/usr/bin/env python3
"""
OpenHardwareMonitor Auto-Manager
Downloads, extracts, and runs OpenHardwareMonitor automatically
"""

import os
import sys
import subprocess
import urllib.request
import zipfile
import time

OHM_URL = "https://openhardwaremonitor.org/files/openhardwaremonitor-v0.9.6.zip"
BACKEND_DIR = os.path.dirname(os.path.abspath(__file__))
OHM_DIR = os.path.join(BACKEND_DIR, "openhardwaremonitor")
OHM_EXE = os.path.join(OHM_DIR, "OpenHardwareMonitor.exe")


def download_and_extract_ohm():
    """Download and extract OpenHardwareMonitor."""
    if os.path.exists(OHM_EXE):
        return True
    
    print("Downloading OpenHardwareMonitor...")
    os.makedirs(OHM_DIR, exist_ok=True)
    
    try:
        zip_path = os.path.join(BACKEND_DIR, "ohm_temp.zip")
        urllib.request.urlretrieve(OHM_URL, zip_path)
        print("✓ Downloaded")
        
        # Extract ZIP
        with zipfile.ZipFile(zip_path, 'r') as zip_ref:
            zip_ref.extractall(BACKEND_DIR)
        
        # Find the exe in extracted files
        for root, dirs, files in os.walk(BACKEND_DIR):
            if "OpenHardwareMonitor.exe" in files:
                extracted_exe = os.path.join(root, "OpenHardwareMonitor.exe")
                
                # If not in our target dir, move everything
                if os.path.dirname(extracted_exe) != OHM_DIR:
                    import shutil
                    shutil.move(os.path.dirname(extracted_exe), OHM_DIR)
                break
        
        os.remove(zip_path)
        
        if os.path.exists(OHM_EXE):
            print(f"✓ Extracted to {OHM_EXE}")
            return True
        else:
            print("✗ Failed to find exe after extraction")
            return False
            
    except Exception as e:
        print(f"✗ Failed: {e}")
        return False


def is_running():
    """Check if OpenHardwareMonitor is running."""
    try:
        result = subprocess.run(
            ['tasklist'], 
            capture_output=True,
            text=True
        )
        return 'OpenHardwareMonitor.exe' in result.stdout
    except:
        return False


def start():
    """Start OpenHardwareMonitor."""
    if is_running():
        print("✓ OpenHardwareMonitor already running")
        return True
    
    if not os.path.exists(OHM_EXE):
        if not download_and_extract_ohm():
            return False
    
    try:
        # Start minimized in background
        subprocess.Popen(
            [OHM_EXE],
            cwd=OHM_DIR,
            creationflags=subprocess.CREATE_NO_WINDOW | subprocess.DETACHED_PROCESS
        )
        
        time.sleep(1)
        print("✓ OpenHardwareMonitor started")
        return True
        
    except Exception as e:
        print(f"✗ Failed to start: {e}")
        return False


def stop():
    """Stop OpenHardwareMonitor."""
    try:
        subprocess.run(
            ['taskkill', '/F', '/IM', 'OpenHardwareMonitor.exe'],
            capture_output=True
        )
        print("✓ Stopped")
    except:
        pass


if __name__ == "__main__":
    if len(sys.argv) > 1 and sys.argv[1] == "--stop":
        stop()
    else:
        success = start()
        sys.exit(0 if success else 1)
