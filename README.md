# ⚡ OmniAIBench

**OmniAIBench** is a free, open-source benchmarking suite designed for the modern hardware landscape. Unlike traditional tools that focus only on raw compute, OmniAIBench is built to test the **AI capabilities** of your system, bridging the gap between standard processors and the emerging world of Neural Processing Units (NPUs).

![License](https://img.shields.io/badge/license-MIT-blue.svg) ![Platform](https://img.shields.io/badge/platform-Windows-lightgrey.svg) ![Status](https://img.shields.io/badge/status-Alpha-orange.svg)

## 🚀 Why OmniAIBench?
Hardware is fragmenting. We now have CPUs (AMD/Intel), GPUs (Nvidia/AMD/Intel), and specialized NPUs (Ryzen AI/Intel Core Ultra). 
OmniAIBench unifies these distinct architectures under one roof. It uses a **dynamic environment orchestrator** to spin up isolated, optimized environments (CUDA, ROCm, OpenVINO, Ryzen AI) to test hardware on its native terms.

## ✨ Key Features

* **🔍 Deep System Inspection:** Detailed hardware scanning (CPU-Z style) that detects standard specs plus hidden AI accelerators (NPU/IPU).
* **🧠 Real AI Workloads:** Don't just crunch numbers. Benchmark your system with actual AI models:
    * **Computer Vision:** ResNet-50, YOLOv8
    * **Generative AI:** Llama-3 (Quantized), BERT
* **🌐 Broad Hardware Support:**
    * **CPU:** AMD Ryzen, Intel Core/Ultra
    * **GPU:** NVIDIA RTX (CUDA), AMD Radeon (DirectML), Intel Arc
    * **NPU:** AMD Ryzen AI (XDNA), Intel Core Ultra (NPU)
* **🛡️ Dynamic Sandbox Engine:** Automatically downloads and creates isolated Python environments (via bundled Micromamba) for each test—no user setup required.
* **📊 Visualization & Leaderboard:** Compare scores via interactive spider charts and upload results to a secure, open leaderboard.

## 🛠️ Architecture

OmniAIBench operates on a unique **"Controller-Worker"** architecture:

1.  **The Shell (Frontend):** Built with **Tauri v2** + **React**, providing a lightweight, secure, and responsive native Windows UI.
2.  **The Orchestrator:** A Rust/Python core that manages the lifecycle of benchmarks.
3.  **The Workers:** Benchmark scripts run in isolated **Micromamba** environments to ensure dependency conflicts (e.g., PyTorch vs. TensorFlow versions) never happen.
4.  **The Engine:** Powered by **ONNX Runtime**, ensuring fair comparisons across different execution providers (CUDA, Vitis AI, OpenVINO).

## 💻 Tech Stack

* **Frontend:** React, TailwindCSS, TypeScript
* **Native Wrapper:** Tauri v2 (Rust)
* **Backend Logic:** Python
* **Environment Management:** Micromamba (bundled)
* **AI Inference:** ONNX Runtime, PyTorch

## 📦 Installation

*(Coming Soon - Alpha builds will be available in Releases)*

## 🤝 Contributing

We welcome contributions! Whether you are an expert in ONNX optimization, a Rust developer, or a UI designer, join us in building the standard for AI benchmarking.

---
*Built with ❤️ by [vikash236]*
