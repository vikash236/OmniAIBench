#!/usr/bin/env python3
"""
Benchmark Engine Module
Unified benchmarking interface for multiple AI frameworks
Supports: ONNX, PyTorch, TensorFlow, OpenVINO
"""

import json
import os
import time
import numpy as np
from typing import Dict, List, Optional


class BenchmarkEngine:
    def __init__(self, model_path: str, framework: Optional[str] = None, provider: str = "CPU"):
        self.model_path = model_path
        self.framework = framework or self._detect_framework(model_path)
        self.provider = provider
        self.session = None
        self.input_shape = None
        
    def _detect_framework(self, path: str) -> str:
        """Detect framework from file extension"""
        ext = os.path.splitext(path)[1].lower()
        framework_map = {
            '.onnx': 'onnx',
            '.pt': 'pytorch',
            '.pth': 'pytorch',
            '.pb': 'tensorflow',
            '.tflite': 'tflite',
            '.xml': 'openvino'
        }
        return framework_map.get(ext, 'onnx')
    
    def load_model(self):
        """Load model based on framework"""
        if self.framework == 'onnx':
            self._load_onnx()
        elif self.framework == 'pytorch':
            self._load_pytorch()
        elif self.framework == 'tensorflow':
            self._load_tensorflow()
        elif self.framework == 'openvino':
            self._load_openvino()
        else:
            raise ValueError(f"Unsupported framework: {self.framework}")
    
    def _load_onnx(self):
        """Load ONNX model"""
        try:
            import onnxruntime as ort
        except ImportError:
            raise ImportError("onnxruntime not installed. Install with: pip install onnxruntime")
        
        # Configure providers based on hardware
        providers = self._get_onnx_providers()
        
        # Create session
        session_options = ort.SessionOptions()
        session_options.log_severity_level = 3  # Error only
        
        self.session = ort.InferenceSession(
            self.model_path,
            sess_options=session_options,
            providers=providers
        )
        
        # Get input shape
        input_info = self.session.get_inputs()[0]
        self.input_shape = input_info.shape
        self.input_name = input_info.name
    
    def _get_onnx_providers(self) -> List:
        """Get ONNX Runtime providers based on selected provider"""
        provider_map = {
            'CUDA': ['CUDAExecutionProvider', 'CPUExecutionProvider'],
            'DirectML': ['DmlExecutionProvider', 'CPUExecutionProvider'],
            'VitisAI': [('VitisAIExecutionProvider', self._get_vitisai_options()), 'CPUExecutionProvider'],
            'OpenVINO': ['OpenVINOExecutionProvider', 'CPUExecutionProvider'],
            'CPU': ['CPUExecutionProvider']
        }
        
        return provider_map.get(self.provider, ['CPUExecutionProvider'])
    
    def _get_vitisai_options(self) -> Dict:
        """Get VitisAI provider options for AMD Ryzen AI NPU"""
        # Detect NPU architecture from hardware_scan
        install_dir = os.path.dirname(os.path.abspath(__file__))
        
        # Default to PHX architecture
        xclbin_file = os.path.join(install_dir, 'voe-4.0-win_amd64', 'xclbins', 'phoenix', '4x4.xclbin')
        
        return {
            'target': 'X1',
            'xlnx_enable_py3_round': 0,
            'xclbin': xclbin_file,
        }
    
    def _load_pytorch(self):
        """Load PyTorch model"""
        try:
            import torch
        except ImportError:
            raise ImportError("PyTorch not installed. Install with: pip install torch")
        
        device = torch.device('cuda' if self.provider == 'CUDA' and torch.cuda.is_available() else 'cpu')
        
        self.session = {
            'model': torch.jit.load(self.model_path),
            'device': device
        }
        self.session['model'].to(device)
        self.session['model'].eval()
    
    def _load_tensorflow(self):
        """Load TensorFlow model"""
        try:
            import tensorflow as tf
        except ImportError:
            raise ImportError("TensorFlow not installed. Install with: pip install tensorflow")
        
        self.session = tf.saved_model.load(self.model_path)
    
    def _load_openvino(self):
        """Load OpenVINO model"""
        try:
            import openvino.runtime as ov
        except ImportError:
            raise ImportError("OpenVINO not installed. Install with: pip install openvino")
        
        core = ov.Core()
        model = core.read_model(self.model_path)
        self.session = core.compile_model(model, 'AUTO')
    
    def run_inference(self, input_data):
        """Framework-agnostic inference"""
        if self.framework == 'onnx':
            return self.session.run(None, {self.input_name: input_data})[0]
        elif self.framework == 'pytorch':
            import torch
            with torch.no_grad():
                tensor = torch.from_numpy(input_data).to(self.session['device'])
                return self.session['model'](tensor).cpu().numpy()
        elif self.framework == 'tensorflow':
            return self.session(input_data).numpy()
        elif self.framework == 'openvino':
            return self.session([input_data])[0]
    
    def benchmark(self, warmup_runs: int = 5, test_runs: int = 50) -> Dict:
        """Run benchmark with warmup"""
        # Load model if not loaded
        if self.session is None:
            self.load_model()
        
        # Generate dummy input
        input_data = self._generate_dummy_input()
        
        # Warmup
        for _ in range(warmup_runs):
            self.run_inference(input_data)
        
        # Benchmark
        times = []
        for _ in range(test_runs):
            start = time.perf_counter()
            self.run_inference(input_data)
            times.append(time.perf_counter() - start)
        
        # Calculate metrics
        avg_time = np.mean(times)
        std_dev = np.std(times)
        latency_ms = avg_time * 1000
        ips = 1.0 / avg_time
        
        # OmniScore: 10000 / latency_ms (higher is better)
        omniscore = int(10000 / latency_ms)
        
        return {
            'framework': self.framework,
            'provider': self.provider,
            'latency_ms': round(latency_ms, 2),
            'std_dev_ms': round(std_dev * 1000, 2),
            'ips': round(ips, 2),
            'omniscore': omniscore,
            'warmup_runs': warmup_runs,
            'test_runs': test_runs,
        }
    
    def _generate_dummy_input(self):
        """Generate dummy input based on model input shape"""
        if self.framework == 'onnx':
            shape = self.input_shape
            # Convert dynamic dimensions to 1
            shape = [dim if isinstance(dim, int) and dim > 0 else 1 for dim in shape]
        else:
            # Default shape for other frameworks
            shape = [1, 3, 224, 224]
        
        return np.random.randn(*shape).astype(np.float32)


def main():
    """CLI for benchmark engine"""
    import argparse
    
    parser = argparse.ArgumentParser(description='Benchmark Engine')
    parser.add_argument('model_path', help='Path to model file')
    parser.add_argument('--framework', help='Framework (onnx, pytorch, tensorflow, openvino)')
    parser.add_argument('--provider', default='CPU', help='Execution provider (CPU, CUDA, DirectML, VitisAI, OpenVINO)')
    parser.add_argument('--warmup', type=int, default=5, help='Warmup runs')
    parser.add_argument('--runs', type=int, default=50, help='Test runs')
    
    args = parser.parse_args()
    
    engine = BenchmarkEngine(
        model_path=args.model_path,
        framework=args.framework,
        provider=args.provider
    )
    
    print(f"Benchmarking {args.model_path} with {args.provider}...")
    results = engine.benchmark(warmup_runs=args.warmup, test_runs=args.runs)
    print(json.dumps(results, indent=2))


if __name__ == "__main__":
    main()
