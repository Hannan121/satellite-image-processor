# Satellite Image Processing Pipeline
### Real-Time Edge Detection System for Space Object Tracking

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![C](https://img.shields.io/badge/C-00599C?logo=c&logoColor=white)](https://en.wikipedia.org/wiki/C_(programming_language))
[![Python](https://img.shields.io/badge/Python-3776AB?logo=python&logoColor=white)](https://www.python.org/)

## Overview

High-performance embedded image processing system designed for satellite-based space object tracking. Implements real-time edge detection pipeline optimized for resource-constrained embedded platforms (AMD Xilinx SoC/FPGA targets).

**Key Achievement**: Processes 3124×3030 pixel images at 10fps with optimized median filtering and edge detection algorithms, demonstrating 37% performance improvement through dual-core parallelization.

## Technical Highlights

### 🚀 Performance Optimizations
- **Single-core**: ~35ms per frame (median: 13ms, edge detection: 22ms)
- **Dual-core**: ~22ms per frame (37% improvement)
- Histogram-based median filtering for O(256) complexity vs O(n log n)
- Separable Gaussian kernels (2×5-tap vs 25-tap convolution)
- Ring buffer architecture with zero-copy descriptor passing

### 🏗️ Architecture
- State machine-driven pipeline with 4-buffer circular queue
- Lock-free inter-process communication using ring descriptors
- Thread affinity management for deterministic execution
- Memory-efficient streaming design suitable for FPGA fabric

### 🎯 Algorithm Implementation
1. **ROI Median Background Subtraction**: 100×100 subframe processing with histogram optimization
2. **Gaussian Noise Reduction**: Binomial approximation (Pascal's triangle) for integer-only arithmetic
3. **Sobel Edge Detection**: Fused gradient computation and inference generation
4. **Inference Output**: Compressed edge pixel list (address + value pairs)

## Quick Start

### Prerequisites
```bash
# Windows (MSYS2/MinGW) - Primary Target Platform
gcc --version  # GCC compiler

# Python dependencies
pip install pillow numpy
```

**Note**: This implementation targets Windows specifically (uses `Windows.h`, `_beginthreadex`, etc.). Linux/POSIX porting is documented but not implemented. See `docs/QUICKSTART.md` for Linux modifications.

### Compilation (Simple - No Makefile Needed)

```bash
# Single-core version
gcc src/main.c src/tasks_v2.c -o satellite_sim.exe -O3

# Dual-core version
gcc src/main_dualcore.c src/tasks_v2.c -o satellite_dualcore_sim.exe -O3
```

**Why -O3?** Provides aggressive compiler optimizations (loop unrolling, auto-vectorization, inlining) without architecture-specific instructions. Keeps code portable and deterministic across different CPUs.

For complete compilation options, see `COMPILE.md`.

### Build & Run

**First time setup (generate test data):**
```bash
python scripts/generate_test_image.py
# Creates both Raw_image_Fig1.jpg and input_image.bin
```

#### Single-Core Version
```bash
# 1. Compile
gcc src/main.c src/tasks_v2.c -o satellite_sim -O3

# 2. Execute
./satellite_sim.exe

# 3. Visualize output
python scripts/view_output.py
```

#### Dual-Core Version
```bash
# 1. Compile dual-core version
gcc src/main_dualcore.c src/tasks_v2.c -o satellite_dualcore_sim -O3

# 2. Execute
./satellite_dualcore_sim.exe

# 3. Visualize output
python scripts/view_output.py
```

**Using Makefile (recommended):**
```bash
make demo  # Generate test image → compile → run → visualize (all in one!)
```

### Output Files
- `output_frame0.pgm` - Edge-detected image (first frame)
- `output_inference0.csv` - Edge pixel coordinates and intensities
- `output_visualized.png` - PNG visualization

## System Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                     Ring Buffer (4 frames)                   │
│  ┌──────────┐   ┌──────────┐   ┌──────────┐   ┌──────────┐ │
│  │ Buffer 0 │──▶│ Buffer 1 │──▶│ Buffer 2 │──▶│ Buffer 3 │ │
│  └──────────┘   └──────────┘   └──────────┘   └──────────┘ │
└─────────────────────────────────────────────────────────────┘
         │                │                │
         ▼                ▼                ▼
    Image Acq.    Median Filter    Edge Detection
  (100ms period)    (Task 1)         (Task 2)
                       │                │
                       └────────────────┘
                    Dual-Core Split (optional)
```

## Algorithm Details

### Median Background Subtraction
- **Input**: 3124×3030 grayscale image
- **Method**: Region-based histogram (100×100 blocks)
- **Complexity**: O(W×H×256) vs O(W×H×n²) for sorting
- **Output**: Background-subtracted image

```c
// Histogram-based median finding
for (each 100×100 block) {
    build_histogram(block);
    median = find_median_from_histogram();
    subtract_median(block, median);
}
```

### Gaussian Smoothing
- **Kernel**: 5×5 binomial approximation [1,4,6,4,1]
- **Method**: Separable convolution (horizontal → vertical)
- **Arithmetic**: Integer-only (sum/256 via bit shift)

### Sobel Edge Detection
- **Gradients**: Gx (vertical edges), Gy (horizontal edges)
- **Magnitude**: |Gx| + |Gy| (Manhattan distance approximation)
- **Threshold**: Configurable (default: 40)
- **Output**: Sparse edge pixel list

## Performance Analysis

### Single-Core Execution (i7-12700F, Windows)
| Frame | Total Time | Median Filter | Edge Detection |
|-------|------------|---------------|----------------|
| 1     | 28.59 ms   | 07.00 ms      | 22.58 ms       |
| 2     | 29.34 ms   | 06.36 ms      | 22.98 ms       |
| 9     | 27.14 ms   | 05.07 ms      | 22.06 ms       |
| Avg   | ~29 ms     | ~06 ms        | ~22.5 ms       |

### Dual-Core Execution
| Frame | Core 0 (Median) | Core 1 (Edge Det.) |
|-------|-----------------|---------------------|
| 1     | 06.19 ms        | 21.89 ms            |
| 2     | 05.99 ms        | 21.67 ms            |
| 10    | 06.13 ms        | 21.52 ms            |
| Avg   | ~06 ms          | ~22 ms              |

**Throughput**: ~30 fps (single-core), ~45 fps (dual-core, bottlenecked by edge detection)

## Hardware Recommendations

### Minimum Spec (3124×3030 @ 10fps)
- **Target**: AMD Xilinx Zynq-7000 (xqr7Z020)
- **Clock**: 118 MHz (94 MHz + 25% safety margin)
- **BRAM**: 2.5 Mb for median ROI buffering
- **Features**: Radiation-hardened, -55°C to 125°C operation

### High-Performance Spec (8000×8000 @ 30fps)
- **Target**: AMD Xilinx Zynq UltraScale+ (ZU9EG)
- **Strategy**: Stream-based FPGA pipeline
- **Architecture**: Camera → FPGA (Median → Gauss → Sobel) → DDR inference only

## Radiation Hardening Strategies

1. **Triple Modular Redundancy (TMR)**: Critical state variables, descriptors, kernel constants
2. **Error Correction Codes (ECC)**: Enable for all memory regions
3. **Software ECC/Parity**: Implement for peripheral configs where hardware ECC unavailable
4. **Watchdog Timers**: Automatic process/peripheral refresh on anomaly detection
5. **Scrubbing**: Periodic readback and correction of configuration memory

## Project Structure

```
satellite-image-processor/
├── src/
│   ├── main.c              # Single-core implementation
│   ├── main_dualcore.c     # Dual-core implementation
│   ├── tasks_v2.c          # Image processing algorithms
│   └── system_def.h        # System configuration & types
├── scripts/
│   ├── convert_jpg_to_bin.py   # Preprocessing tool
│   └── view_output.py          # Visualization tool
├── docs/
│   └── IMPLEMENTATION.md       # Detailed algorithm documentation
├── test/
│   └── Raw_image_Fig1.jpg      # Sample test image (not included)
└── README.md
```

## Configuration

Edit `system_def.h` to modify:
```c
#define IMG_WIDTH 3124          // Image width
#define IMG_HEIGHT 3030         // Image height
#define BUFF_SIZE 4             // Ring buffer size
#define MEDIAN_SUBFRAME_SIZE 100 // Median ROI size
#define EDGE_THRESHOLD 40       // Edge detection threshold
#define MAX_EDGES 50000         // Max inference list size
```

## Future Enhancements

- [ ] Manual SIMD optimization (SSE/AVX intrinsics) - *Current -O3 auto-vectorization is sufficient*
- [ ] FPGA HDL implementation (Verilog/VHDL)
- [ ] Adaptive thresholding (histogram-based)
- [ ] Combined Gauss-Sobel kernel fusion
- [ ] Horizontal tiling for better cache locality
- [ ] AXI DMA integration for Xilinx platforms
- [ ] Real-time telemetry streaming
- [ ] Linux/POSIX support - *Currently Windows-only by design*

## Technical Decisions & Rationale

### Why -O3 Optimization?
Compiler-driven optimization provides excellent performance without architecture-specific code:
- **Auto-vectorization**: GCC automatically applies SIMD where beneficial
- **Portable**: Same code runs on different CPUs (Intel, AMD, ARM)
- **Deterministic**: No manual assembly or intrinsics to maintain
- **Good enough**: Achieves 35ms single-core, 22ms dual-core performance

Manual SIMD (SSE/AVX) is a future enhancement, but current performance meets 10fps requirements with safety margin.

### Why Histogram-Based Median?
Traditional median finding requires sorting (O(n log n)). For a 100×100 region:
- Sorting: ~66,000 operations
- Histogram: ~10,000 operations (single pass + 256 accumulation)

### Why Separable Gaussian?
A 5×5 convolution requires 25 multiplications per pixel. Separable approach:
- 5 multiplications (horizontal) + 5 (vertical) = 10 total
- 2.5× fewer operations

### Why Binomial Approximation?
True Gaussian requires floating-point or fixed-point division. Binomial [1,4,6,4,1]:
- Sum = 16 → division via `>> 4` (bit shift)
- Extended to 2D: sum = 256 → division via `>> 8`
- 100% integer arithmetic, no FPU required

## License

MIT License - See LICENSE file for details

## Authors and Acknowledgments

Mohamed Hannan Sohail  
[mdhannansohail141@gmail.com](mailto:mdhannansohail141@gmail.com)

**Primary Author & Implementation:**
Mohamed Hannan Sohail - All image processing algorithms, system architecture, performance optimization, and core C implementation.

**Documentation & Tooling:**
Documentation structure, README formatting, Makefile, synthetic test generator, and interview preparation materials created with assistance from Claude (Anthropic AI).

**Research & Learning:**
Claude (Anthropic AI) was used to research and understand AMD Xilinx SoC architectures, FPGA implementation strategies, and image filtering techniques (Gaussian smoothing, Sobel operators) during the development of this project.


**Note:** The technical implementation, algorithmic decisions, and performance optimizations are entirely the work of the primary author.


## Acknowledgments

This project was developed as a technical demonstration of embedded systems expertise for satellite image processing applications. The implementation focuses on algorithm correctness and architectural design suitable for resource-constrained embedded platforms.

---

**Note**: This is a portfolio demonstration project. Test images not included due to potential proprietary nature. Use your own test images or generate synthetic star field data for testing.
