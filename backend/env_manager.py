#!/usr/bin/env python3
"""
Environment Manager Module
Manages Micromamba environments for different hardware configurations
"""

import json
import os
import platform
import subprocess
import sys
from pathlib import Path


class EnvironmentManager:
    def __init__(self, install_dir=None):
        self.install_dir = install_dir or os.path.dirname(os.path.abspath(__file__))
        self.micromamba_exe = self._find_micromamba()
        
    def _find_micromamba(self):
        """Find micromamba executable"""
        if platform.system() == "Windows":
            return os.path.join(self.install_dir, "micromamba", "micromamba.exe")
        else:
            return os.path.join(self.install_dir, "micromamba", "micromamba")
    
    def get_required_environments(self, hardware_info):
        """Determine which environments are needed based on hardware"""
        environments = []
        
        # CPU environment (always included)
        environments.append({
            'name': 'env_cpu',
            'packages': ['onnxruntime', 'numpy', 'pillow'],
            'channels': ['conda-forge'],
            'size_estimate': '500MB',
            'time_estimate': '2-3 min'
        })
        
        # CUDA environment for NVIDIA GPUs
        if hardware_info.get('gpu', {}).get('name') and 'NVIDIA' in hardware_info['gpu']['name']:
            environments.append({
                'name': 'env_cuda',
                'packages': [
                    'onnxruntime-gpu',
                    'torch',
                    'torchvision',
                    'numpy',
                    'pillow',
                    'cudatoolkit=11.8'
                ],
                'channels': ['pytorch', 'nvidia', 'conda-forge'],
                'size_estimate': '2.5GB',
                'time_estimate': '5-8 min'
            })
        
        # DirectML environment for Windows GPU acceleration
        if hardware_info.get('os') == 'Windows' and hardware_info.get('gpu', {}).get('name'):
            environments.append({
                'name': 'env_directml',
                'packages': ['onnxruntime-directml', 'numpy', 'pillow'],
                'channels': ['conda-forge'],
                'size_estimate': '800MB',
                'time_estimate': '3-4 min'
            })
        
        # AMD Ryzen AI NPU environment
        npu_info = hardware_info.get('npu', {})
        if npu_info.get('name') and 'AMD' in npu_info['name']:
            architecture = npu_info.get('architecture', 'PHX')
            environments.append({
                'name': 'env_ryzen_ai',
                'packages': [
                    'onnxruntime-vitisai',
                    'numpy',
                    'pillow'
                ],
                'channels': ['amd', 'conda-forge'],
                'architecture': architecture,
                'xclbin_path': f'xclbins/{architecture.lower()}/4x4.xclbin',
                'size_estimate': '1.8GB',
                'time_estimate': '6-8 min'
            })
        
        # Intel NPU environment
        if npu_info.get('name') and 'Intel' in npu_info['name']:
            environments.append({
                'name': 'env_intel_npu',
                'packages': [
                    'openvino',
                    'openvino-dev',
                    'numpy',
                    'pillow'
                ],
                'channels': ['conda-forge'],
                'size_estimate': '1.5GB',
                'time_estimate': '4-6 min'
            })
        
        return environments
    
    def create_environment(self, env_config, progress_callback=None):
        """Create a micromamba environment"""
        env_name = env_config['name']
        packages = env_config['packages']
        channels = env_config.get('channels', ['conda-forge'])
        
        if progress_callback:
            progress_callback(f"Creating environment: {env_name}")
        
        # Build micromamba command
        cmd = [
            self.micromamba_exe,
            'create',
            '-n', env_name,
            '-y',
            '--quiet'
        ]
        
        # Add channels
        for channel in channels:
            cmd.extend(['-c', channel])
        
        # Add packages
        cmd.extend(packages)
        
        try:
            # Execute command
            process = subprocess.Popen(
                cmd,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
                bufsize=1
            )
            
            # Stream output
            for line in process.stdout:
                if progress_callback:
                    progress_callback(line.strip())
            
            process.wait()
            
            if process.returncode == 0:
                if progress_callback:
                    progress_callback(f"✓ Environment {env_name} created successfully")
                return True
            else:
                error = process.stderr.read()
                if progress_callback:
                    progress_callback(f"✗ Failed to create {env_name}: {error}")
                return False
                
        except Exception as e:
            if progress_callback:
                progress_callback(f"✗ Error creating {env_name}: {str(e)}")
            return False
    
    def list_environments(self):
        """List all created environments"""
        try:
            result = subprocess.run(
                [self.micromamba_exe, 'env', 'list'],
                capture_output=True,
                text=True
            )
            return result.stdout
        except Exception as e:
            return f"Error listing environments: {str(e)}"
    
    def activate_environment(self, env_name):
        """Get activation command for environment"""
        if platform.system() == "Windows":
            return f"{self.micromamba_exe} activate {env_name}"
        else:
            return f"eval \"$({self.micromamba_exe} shell activate {env_name})\""


def main():
    """CLI for environment manager"""
    import argparse
    
    parser = argparse.ArgumentParser(description='Environment Manager')
    parser.add_argument('--get-required', help='Get required environments from hardware JSON')
    parser.add_argument('--create', help='Create environment by name')
    parser.add_argument('--list', action='store_true', help='List environments')
    
    args = parser.parse_args()
    
    manager = EnvironmentManager()
    
    if args.get_required:
        hardware_info = json.loads(args.get_required)
        environments = manager.get_required_environments(hardware_info)
        print(json.dumps(environments, indent=2))
    
    elif args.create:
        # This would need env config passed in
        print(f"Create functionality requires full env config")
    
    elif args.list:
        print(manager.list_environments())


if __name__ == "__main__":
    main()
