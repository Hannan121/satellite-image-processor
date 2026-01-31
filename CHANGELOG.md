# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.0.0] - 2025-01-30

### Added
- Initial release of satellite image processing pipeline
- Single-core implementation (main.c)
- Dual-core implementation (main_dualcore.c)
- Image processing algorithms:
  - Region-based median background subtraction (100×100 ROI with histogram)
  - 5×5 Gaussian noise reduction (separable binomial approximation)
  - 3×3 Sobel edge detection with magnitude calculation
- Ring buffer architecture with state machine
- Thread affinity support for deterministic execution
- Python utilities:
  - JPG to binary converter (convert_jpg_to_bin.py)
  - Output visualization tool (view_output.py)
  - Synthetic test image generator (generate_test_image.py)
- Comprehensive documentation:
  - README with quick start guide
  - Implementation details (IMPLEMENTATION.md)
  - Quick start guide (QUICKSTART.md)
- Performance benchmarking framework
- Edge pixel inference generation

### Performance
- Single-core: ~35ms per frame (3124×3030 @ 10fps capable)
- Dual-core: ~22ms per frame (37% improvement)
- Histogram-based median: O(256) vs O(n log n) traditional approach
- Separable Gaussian: 2.5× fewer operations vs standard convolution

### Configuration
- Configurable image dimensions (system_def.h)
- Adjustable edge detection threshold
- Configurable ring buffer size (2-8 buffers)
- Tunable median ROI size

### Platform Support
- Windows (primary target) with MinGW/MSVC
- Linux support via POSIX modifications (documented)
- Targets AMD Xilinx Zynq-7000/UltraScale+ SoC platforms

### Documentation
- Algorithm explanations with complexity analysis
- Memory layout and optimization strategies
- Hardware recommendations for space-grade applications
- Radiation hardening mitigation techniques
- FPGA implementation considerations

## [Unreleased]

### Planned Features
- SIMD optimizations (SSE/AVX intrinsics)
- Horizontal tiling for better load balancing
- Combined Gauss-Sobel kernel fusion
- Adaptive thresholding (Otsu's method)
- FPGA HDL implementation (Verilog)
- AXI DMA integration for Xilinx platforms
- Real-time telemetry streaming
- Linux/POSIX full support
- Unit test suite
- CI/CD pipeline

### Known Issues
- Windows-only threading (requires porting for cross-platform)
- Large static memory allocation (~105MB)
- Dual-core limited by slowest task (edge detection)
- No dynamic threshold adjustment

---

## Version History

**v1.0.0** (2025-01-30) - Initial public release
