#!/usr/bin/env python3
"""
Model Downloader Module
Downloads AI models on-demand with progress tracking
Supports multiple frameworks: ONNX, PyTorch, TensorFlow, OpenVINO
"""

import json
import os
import urllib.request
from pathlib import Path
from typing import Optional, Dict, List


# Model catalog with all available models
MODEL_CATALOG = {
    # Computer Vision - ResNet50
    "resnet50_onnx": {
        "name": "ResNet50",
        "framework": "onnx",
        "precision": "FP32",
        "category": "Computer Vision",
        "vram_gb": 2,
        "ram_gb": 4,
        "size_mb": 98,
        "url": "https://github.com/onnx/models/raw/main/vision/classification/resnet/model/resnet50-v2-7.onnx",
        "input_shape": [1, 3, 224, 224],
    },
    "resnet50_fp16_onnx": {
        "name": "ResNet50",
        "framework": "onnx",
        "precision": "FP16",
        "category": "Computer Vision",
        "vram_gb": 1,
        "ram_gb": 2,
        "size_mb": 51,
        "url": "https://github.com/onnx/models/raw/main/vision/classification/resnet/model/resnet50-v2-7-fp16.onnx",
        "input_shape": [1, 3, 224, 224],
    },
    
    # Object Detection - YOLOv8
    "yolov8n_onnx": {
        "name": "YOLOv8n",
        "framework": "onnx",
        "precision": "FP32",
        "category": "Object Detection",
        "vram_gb": 1,
        "ram_gb": 2,
        "size_mb": 6,
        "url": "https://github.com/ultralytics/assets/releases/download/v0.0.0/yolov8n.onnx",
        "input_shape": [1, 3, 640, 640],
    },
    "yolov8s_onnx": {
        "name": "YOLOv8s",
        "framework": "onnx",
        "precision": "FP32",
        "category": "Object Detection",
        "vram_gb": 2,
        "ram_gb": 4,
        "size_mb": 22,
        "url": "https://github.com/ultralytics/assets/releases/download/v0.0.0/yolov8s.onnx",
        "input_shape": [1, 3, 640, 640],
    },
    
    # LLMs - Phi-3-mini (quantized variants)
    "phi3_mini_int4": {
        "name": "Phi-3-mini",
        "framework": "onnx",
        "precision": "INT4",
        "category": "LLM",
        "vram_gb": 3,
        "ram_gb": 8,
        "size_mb": 2048,
        "url": "https://huggingface.co/microsoft/Phi-3-mini-4k-instruct-onnx/resolve/main/cpu_and_mobile/cpu-int4-rtn-block-32-acc-level-4/phi3-mini-4k-instruct-cpu-int4-rtn-block-32-acc-level-4.onnx",
        "warning_level": 1,
    },
}


class ModelDownloader:
    def __init__(self, cache_dir=None):
        self.cache_dir = cache_dir or os.path.join(os.path.dirname(__file__), "cache", "models")
        Path(self.cache_dir).mkdir(parents=True, exist_ok=True)
    
    def list_models(self, framework: Optional[str] = None, category: Optional[str] = None) -> List[Dict]:
        """List available models, optionally filtered by framework or category"""
        models = []
        for model_id, info in MODEL_CATALOG.items():
            if framework and info['framework'] != framework:
                continue
            if category and info['category'] != category:
                continue
            
            models.append({
                'id': model_id,
                **info
            })
        
        return models
    
    def get_hardware_requirements(self, model_id: str) -> Dict:
        """Get hardware requirements for a model"""
        if model_id not in MODEL_CATALOG:
            raise ValueError(f"Model {model_id} not found in catalog")
        
        model_info = MODEL_CATALOG[model_id]
        return {
            'vram_gb': model_info.get('vram_gb', 0),
            'ram_gb': model_info.get('ram_gb', 0),
            'warning_level': model_info.get('warning_level', 0),
        }
    
    def warn_if_insufficient(self, model_id: str, user_hardware: Dict) -> str:
        """Check if user's hardware meets requirements and return warnings"""
        requirements = self.get_hardware_requirements(model_id)
        model_info = MODEL_CATALOG[model_id]
        
        user_vram = user_hardware.get('vram_gb', 0)
        user_ram = user_hardware.get('ram_gb', 0)
        
        warnings = []
        
        if user_vram < requirements['vram_gb']:
            warnings.append(
                f"⚠️ WARNING: {model_info['name']} requires {requirements['vram_gb']}GB VRAM, "
                f"but you have {user_vram}GB. Benchmark may fail or run extremely slowly."
            )
        
        if user_ram < requirements['ram_gb']:
            warnings.append(
                f"⚠️ WARNING: {model_info['name']} requires {requirements['ram_gb']}GB RAM, "
                f"but you have {user_ram}GB. System may become unresponsive."
            )
        
        warning_level = requirements['warning_level']
        if warning_level >= 2 and user_vram < 8:
            warnings.append(
                f"🚨 CAUTION: {model_info['name']} is a high-end model designed for enthusiast/workstation GPUs. "
                f"It may not run on your current hardware."
            )
        
        if warning_level >= 3:
            warnings.append(
                f"🚨 EXTREME: {model_info['name']} requires data center/multi-GPU setups. "
                f"It is NOT recommended for consumer hardware."
            )
        
        return '\n'.join(warnings) if warnings else ''
    
    def download_model(self, model_id: str, progress_callback=None) -> str:
        """Download a model and return its path"""
        if model_id not in MODEL_CATALOG:
            raise ValueError(f"Model {model_id} not found in catalog")
        
        model_info = MODEL_CATALOG[model_id]
        filename = f"{model_id}.{model_info['framework']}"
        filepath = os.path.join(self.cache_dir, filename)
        
        # Check if already downloaded
        if os.path.exists(filepath):
            if progress_callback:
                progress_callback(f"✓ Model {model_info['name']} already cached")
            return filepath
        
        # Download model
        if progress_callback:
            progress_callback(f"Downloading {model_info['name']} ({model_info['size_mb']}MB)...")
        
        def download_progress(block_num, block_size, total_size):
            downloaded = block_num * block_size
            percent = min(100, (downloaded / total_size) * 100) if total_size > 0 else 0
            if progress_callback and block_num % 100 == 0:
                progress_callback(f"Progress: {percent:.1f}%")
        
        try:
            urllib.request.urlretrieve(
                model_info['url'],
                filepath,
                reporthook=download_progress
            )
            
            if progress_callback:
                progress_callback(f"✓ Downloaded {model_info['name']}")
            
            return filepath
            
        except Exception as e:
            if progress_callback:
                progress_callback(f"✗ Failed to download {model_info['name']}: {str(e)}")
            raise
    
    def verify_model(self, model_path: str) -> bool:
        """Verify that a model file exists and is valid"""
        if not os.path.exists(model_path):
            return False
        
        # Basic size check
        size = os.path.getsize(model_path)
        if size < 1024:  # Less than 1KB is suspicious
            return False
        
        return True


def main():
    """CLI for model downloader"""
    import argparse
    
    parser = argparse.ArgumentParser(description='Model Downloader')
    parser.add_argument('--list', action='store_true', help='List all models')
    parser.add_argument('--download', help='Download model by ID')
    parser.add_argument('--framework', help='Filter by framework')
    parser.add_argument('--category', help='Filter by category')
    
    args = parser.parse_args()
    
    downloader = ModelDownloader()
    
    if args.list:
        models = downloader.list_models(framework=args.framework, category=args.category)
        print(json.dumps(models, indent=2))
    
    elif args.download:
        def progress(msg):
            print(msg)
        
        path = downloader.download_model(args.download, progress_callback=progress)
        print(f"Model saved to: {path}")


if __name__ == "__main__":
    main()
