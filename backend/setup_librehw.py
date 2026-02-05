#!/usr/bin/env python3
"""
LibreHardwareMonitor Auto-Downloader
Downloads and extracts LibreHardwareMonitor DLL for Python integration
"""

import os
import sys
import urllib.request
import zipfile

LHM_URL = "https://github.com/LibreHardwareMonitor/LibreHardwareMonitor/releases/download/v0.9.3/LibreHardwareMonitor-net472.zip"
BACKEND_DIR = os.path.dirname(os.path.abspath(__file__))
LHM_DIR = os.path.join(BACKEND_DIR, "LibreHardwareMonitor")
DLL_PATH = os.path.join(LHM_DIR, "LibreHardwareMonitorLib.dll")


def download_lhm():
    """Download and extract LibreHardwareMonitor."""
    if os.path.exists(DLL_PATH):
        print(f"✓ LibreHardwareMonitor already installed at {LHM_DIR}")
        return True
    
    print("Downloading LibreHardwareMonitor...")
    os.makedirs(LHM_DIR, exist_ok=True)
    
    try:
        zip_path = os.path.join(BACKEND_DIR, "lhm_temp.zip")
        urllib.request.urlretrieve(LHM_URL, zip_path)
        print("✓ Downloaded")
        
        # Extract ZIP
        with zipfile.ZipFile(zip_path, 'r') as zip_ref:
            zip_ref.extractall(LHM_DIR)
        
        os.remove(zip_path)
        
        if os.path.exists(DLL_PATH):
            print(f"✓ Installed to {LHM_DIR}")
            return True
        else:
            print("✗ DLL not found after extraction")
            return False
            
    except Exception as e:
        print(f"✗ Failed: {e}")
        return False


def install_pythonnet():
    """Install pythonnet package."""
    try:
        import clr
        print("✓ pythonnet already installed")
        return True
    except ImportError:
        print("Installing pythonnet...")
        import subprocess
        subprocess.check_call([sys.executable, "-m", "pip", "install", "pythonnet"])
        print("✓ pythonnet installed")
        return True


if __name__ == "__main__":
    print("=== LibreHardwareMonitor Setup ===\n")
    
    # Install pythonnet
    if not install_pythonnet():
        sys.exit(1)
    
    # Download LHM
    if not download_lhm():
        sys.exit(1)
    
    print("\n✓ Setup complete! Hardware monitoring ready.")
