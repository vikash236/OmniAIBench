# OmniAIBench

<div align="center">

![OmniAIBench Logo](https://img.shields.io/badge/OmniAIBench-v1.0.0-7dcfff?style=for-the-badge)

**Comprehensive AI & Hardware Benchmarking Suite**

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Tauri](https://img.shields.io/badge/Tauri-2.0-24C8D8?style=flat-square&logo=tauri)](https://tauri.app/)
[![React](https://img.shields.io/badge/React-18.2-61DAFB?style=flat-square&logo=react)](https://reactjs.org/)
[![TypeScript](https://img.shields.io/badge/TypeScript-5.3-3178C6?style=flat-square&logo=typescript)](https://www.typescriptlang.org/)

A next-generation benchmarking application combining **Geekbench-style CPU/GPU compute tests** + **3DMark-inspired graphics benchmarks** + **cutting-edge AI workload testing**.

[Features](#features) • [Installation](#installation) • [Usage](#usage) • [Roadmap](#roadmap) • [Contributing](#contributing)

</div>

---

## 🚀 Features

### 🔥 Core Capabilities

- **CPU Benchmarking**: Single-core, multi-core, AR/ML workloads with Geekbench-style scoring
- **GPU Compute**: OpenCL, Vulkan, DirectX 12 tests for image processing, matrix operations
- **AI Inference**: Multi-framework support (ONNX, PyTorch, TensorFlow, OpenVINO)
- **NPU Support**: AMD Ryzen AI (Phoenix/Hawk Point/Strix), Intel AI Boost
- **Stress Testing**: CPU/GPU/Memory stability testing with thermal monitoring
- **Gaming FPS Estimates**: Predict performance in popular games based on scores
- **Global Leaderboard**: Compare your results with similar hardware worldwide

### 💎 What Makes It Special

| Feature | OmniAIBench | Geekbench | 3DMark | Cinebench |
|---------|-------------|-----------|--------|-----------|
| CPU Benchmarks | ✅ | ✅ | ❌ | ✅ |
| GPU Compute | ✅ | ✅ | ❌ | ❌ |
| **NPU Benchmarks** | ✅ | ❌ | ❌ | ❌ |
| **AI Inference** | ✅ Multi-framework | ❌ | ❌ | ❌ |
| Stress Testing | ✅ | ❌ | Partial | ❌ |
| Real-time Monitoring | ✅ | ❌ | ✅ | ❌ |
| **Open Source** | ✅ MIT | ❌ | ❌ | ❌ |

---

## 📦 Installation

### Prerequisites

- **Windows 10/11** (Linux/macOS coming soon)
- **Python 3.10+** (bundled in installer)
- **8GB RAM** minimum (16GB+ recommended for AI benchmarks)

### Quick Start

1. Download the latest release from [Releases](https://github.com/yourusername/OmniAIBench/releases)
2. Run the installer (`OmniAIBench-Setup.exe`)
3. The app will automatically detect your hardware and set up required environments

### From Source

```bash
# Clone repository
git clone https://github.com/yourusername/OmniAIBench.git
cd OmniAIBench

# Install frontend dependencies
npm install

# Install Python backend dependencies
pip install -r backend/requirements.txt

# Run development server
npm run tauri dev
```

---

## 🎯 Usage

### Running Benchmarks

1. **Quick Benchmark** (5 minutes): Tests CPU, GPU, and AI in one go
2. **CPU Bench**: Comprehensive single/multi-core tests
3. **GPU Compute**: OpenCL/Vulkan tests for your graphics card
4. **NPU Neural**: AI inference on AMD Ryzen AI or Intel NPU
5. **Stress Test**: System stability validation with thermal monitoring

### Model Selection

Choose from **40+ AI models**:
- **Computer Vision**: ResNet50, EfficientNet, ViT
- **Object Detection**: YOLOv8 (nano to xlarge)
- **LLMs**: Phi-3-mini, Llama-3 (FP32, FP16, INT8, INT4)
- **Segmentation**: SAM (Segment Anything)
- **Audio**: Whisper

⚠️ The app warns you if your hardware is insufficient for high-end models.

---

## 🗺️ Roadmap

**Phase 1** (Current): Core infrastructure
- ✅ CPU/GPU compute benchmarks
- ✅ AI inference testing
- ✅ NPU support (AMD, Intel)
- ✅ Leaderboard system

**Phase 2** (Next 6 months):
- Graphics rendering benchmarks (DirectX 12, Vulkan)
- Ray tracing tests
- Mobile platform support (Android/iOS)
- Power efficiency scoring

**Phase 3** (1 year):
- Cloud benchmark orchestration
- Multi-system comparison dashboard
- Hardware failure prediction
- OEM pre-install testing integration

---

## 🛠️ Technology Stack

- **Frontend**: Tauri 2.0, React 18, TypeScript, Tailwind CSS, Shadcn/UI
- **Backend**: Rust (Tauri), Python 3.10+
- **AI Frameworks**: ONNX Runtime, PyTorch, TensorFlow, OpenVINO
- **Databases**: Firebase (auth), Supabase (leaderboard)
- **UI Theme**: TokyoNight with glassmorphism

---

## 🤝 Contributing

Contributions are welcome! Please read our [Contributing Guide](CONTRIBUTING.md) first.

### Development Setup

```bash
# Install Rust
winget install --id=Rustlang.Rustup -e

# Install Node.js
winget install -e --id OpenJS.NodeJS

# Install dependencies
npm install
cargo build

# Run dev server
npm run tauri dev
```

---

## 📄 License

This project is licensed under the **MIT License** - see the [LICENSE](LICENSE) file for details.

---

## 🙏 Acknowledgments

- Inspired by [Geekbench](https://www.geekbench.com/) and [3DMark](https://www.3dmark.com/)
- UI design inspired by [TokyoNight](https://github.com/enkia/tokyo-night-vscode-theme)
- Built with [Tauri](https://tauri.app/), [React](https://reactjs.org/), and [Shadcn/UI](https://ui.shadcn.com/)

---

<div align="center">

**Made with ❤️ by the OmniAIBench team**

[Report Bug](https://github.com/yourusername/OmniAIBench/issues) • [Request Feature](https://github.com/yourusername/OmniAIBench/issues) • [Join Discord](https://discord.gg/omniaibench)

</div>
