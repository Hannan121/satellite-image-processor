# Implementation Details

## System Architecture

### Ring Buffer Design

The system uses a descriptor-based ring buffer to manage image frames through the processing pipeline. Each descriptor contains:

```c
typedef struct img_ll_struct {
    struct img_ll_struct* nxt_ptr;      // Next buffer in ring
    uint8_t* raw_img_ptr;               // Raw input image
    uint8_t* processed_img_ptr;         // Processed output image
    buffer_state_t status;              // Current processing state
    inference_list_t* inference_ptr;    // Edge detection results
} img_ll_struct_t;
```

### State Machine

```
WRITE_READY → Image Acquisition loads data
     ↓
TASK1_READY → Median filter processes
     ↓
TASK2_READY → Edge detection processes
     ↓
TASK2_COMPLETE → Results available, return to WRITE_READY
```

## Algorithm Deep Dive

### 1. Median Background Subtraction

**Goal**: Remove slowly-varying background illumination while preserving edges.

**Traditional Approach**: For each pixel, sort neighborhood → O(n² log n²) per pixel
**Our Approach**: Region-based histogram → O(n² × 256) total

#### Implementation Steps:

```c
for each 100×100 region:
    1. Clear histogram[256]
    2. Build histogram:
       for each pixel in region:
           histogram[pixel_value]++
    3. Find median:
       cumsum = 0
       for i = 0 to 255:
           cumsum += histogram[i]
           if cumsum >= total_pixels/2:
               median = i
               break
    4. Subtract median from all pixels in region:
       output_pixel = max(0, input_pixel - median)
```

**Boundary Handling**: Edge regions may be smaller than 100×100. We handle this by computing actual region size:
```c
y_end = min(y_start + 100, IMG_HEIGHT)
x_end = min(x_start + 100, IMG_WIDTH)
```

**Memory Efficiency**: Single histogram array (256 × 4 bytes = 1KB) reused for all regions.

### 2. Gaussian Noise Reduction

**Goal**: Smooth noise while preserving edge structure.

**Standard Gaussian 5×5**: Requires 25 floating-point multiplications per pixel.

**Our Binomial Approximation**:
```
[1, 4, 6, 4, 1] × [1, 4, 6, 4, 1]ᵀ = [1  4  6  4  1]
                                      [4 16 24 16  4]
                                      [6 24 36 24  6]
                                      [4 16 24 16  4]
                                      [1  4  6  4  1]
```

Sum of all coefficients = 256 → Division via `>> 8` (bit shift)

**Separable Implementation**:
```c
// Horizontal pass (store intermediate results)
for y = 0 to HEIGHT:
    for x = 2 to WIDTH-2:
        sum = img[y][x-2]*1 + img[y][x-1]*4 + img[y][x]*6 
            + img[y][x+1]*4 + img[y][x+2]*1
        gauss_hor[y][x] = sum  // 16-bit intermediate

// Vertical pass (final output)
for y = 2 to HEIGHT-2:
    for x = 2 to WIDTH-2:
        sum = gauss_hor[y-2][x]*1 + gauss_hor[y-1][x]*4 
            + gauss_hor[y][x]*6 + gauss_hor[y+1][x]*4 
            + gauss_hor[y+2][x]*1
        output[y][x] = (sum + 128) >> 8  // Round and divide
```

**Why +128 before shift?** Rounding: without it, 255.9 becomes 255 (floor). With +128, values round to nearest integer.

**Trade-offs**:
- ✅ Integer-only arithmetic (no FPU required)
- ✅ 10 operations vs 25 per pixel (2.5× faster)
- ✅ Approximates Gaussian (σ ≈ 1.0)
- ⚠️ Requires intermediate buffer (3124×3030×2 bytes = 19MB)

### 3. Sobel Edge Detection

**Goal**: Detect edges by finding local gradients.

**Sobel Operators**:
```
Gx (vertical edges):     Gy (horizontal edges):
[-1  0  1]               [-1 -2 -1]
[-2  0  2]               [ 0  0  0]
[-1  0  1]               [ 1  2  1]
```

**Gradient Magnitude**: `|Gx| + |Gy|` (Manhattan distance approximation of √(Gx² + Gy²))

**Implementation**:
```c
for y = margin to HEIGHT-margin:
    for x = margin to WIDTH-margin:
        // Gx calculation
        gx = (img[y-1][x+1] - img[y-1][x-1])
           + 2*(img[y][x+1] - img[y][x-1])
           + (img[y+1][x+1] - img[y+1][x-1])
        
        // Gy calculation
        gy = (img[y+1][x-1] + 2*img[y+1][x] + img[y+1][x+1])
           - (img[y-1][x-1] + 2*img[y-1][x] + img[y-1][x+1])
        
        magnitude = abs(gx) + abs(gy)
        
        // Store result
        output[y][x] = clamp(magnitude, 0, 255)
        
        // Inference generation (fused for efficiency)
        if magnitude >= THRESHOLD:
            save_to_inference_list(y*WIDTH + x, magnitude)
```

**Memory Optimization**: Sobel and inference generation happen in same loop to avoid re-reading image data.

**Threshold Selection**: Static threshold (40) chosen empirically. Dynamic thresholding (e.g., Otsu's method) could improve results but adds computational cost.

## Dual-Core Parallelization

### Design Challenges

**Problem**: Task 2 (22ms) takes longer than Task 1 (13ms)
**Implication**: Task 1 will stall waiting for Task 2 to free buffers

**Solution**: Producer-consumer pattern with ring buffer

### Thread Affinity

```c
// Core 0: Median filtering
SetThreadAffinityMask(GetCurrentThread(), 0x01);  // CPU 0

// Core 1: Edge detection
SetThreadAffinityMask(GetCurrentThread(), 0x02);  // CPU 1
```

**Why this matters**: Prevents OS from migrating threads, which causes:
- Cache invalidation
- Non-deterministic execution times
- Memory latency spikes

### Synchronization

**No mutexes required!** State machine provides lock-free synchronization:
```
Core 0: if(buffer->status == WRITE_READY) → process → buffer->status = TASK1_READY
Core 1: if(buffer->status == TASK2_READY) → process → buffer->status = WRITE_READY
```

**Buffer Starvation Handling**:
```c
if (buffer->status != EXPECTED_STATE) {
    printf("[ERROR] Buffer not free\n");
    Sleep(1);  // Back off and retry
}
```

### Performance Analysis

**Single Core**: Sequential execution
- Frame time = Median (13ms) + Edge (22ms) = 35ms
- Throughput = 28.6 fps

**Dual Core**: Parallel execution
- Frame time = max(Median, Edge) = 22ms
- Throughput = 45.5 fps
- Improvement = 37%

**Why not 2× faster?** Edge detection is the bottleneck. To achieve 2× speedup:
1. Split edge detection across both cores (horizontal tiling)
2. Implement FPGA acceleration for edge detection
3. Use SIMD instructions (SSE/AVX)

## Memory Layout

### Static Allocation
```c
static uint8_t raw_img[4][3030][3124];      // 38MB
static uint8_t processed_img[4][3030][3124]; // 38MB
static uint16_t gauss_hor_out[3030][3124];   // 19MB
static uint8_t gauss_out[3030][3124];        // 9.5MB
                                    Total:  ~105MB
```

**Design Choice**: Static allocation eliminates:
- Runtime allocation overhead
- Memory fragmentation
- Non-deterministic malloc() calls

**Trade-off**: Large memory footprint, but acceptable for satellite platforms with DDR.

### Cache Optimization Opportunities

**Current**: Row-major processing (good for horizontal pass, poor for vertical)
**Improvement**: Tiled processing (e.g., 64×64 tiles) to improve cache locality

## Platform-Specific Considerations

### Windows Dependencies
- `Windows.h` for `QueryPerformanceCounter()` timing
- `_beginthreadex()` for thread creation
- `SetThreadAffinityMask()` for CPU pinning

**Portability**: Replace with POSIX equivalents for Linux:
```c
// Timing
clock_gettime(CLOCK_MONOTONIC, &ts);

// Threading
pthread_create(&thread, NULL, task_func, NULL);

// CPU affinity
pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
```

### FPGA/SoC Considerations

**For Xilinx Zynq Implementation**:
1. Processor runs state machine and descriptor management
2. FPGA fabric implements compute kernels (median, Gauss, Sobel)
3. AXI DMA for zero-copy transfers between PS and PL

**Streaming Architecture**:
```
Camera → AXI-Stream → Median (PL) → Gauss (PL) → Sobel (PL) → Inference FIFO → DDR
                                                                         ↓
                                                                   ARM Processor
```

## Optimization Roadmap

### 1. SIMD Vectorization
Current: Scalar operations (1 pixel at a time)
Target: Process 16 pixels simultaneously (SSE/AVX)

**Example** (horizontal Gaussian):
```c
// Current: scalar
for (x = 0; x < WIDTH; x++)
    output[x] = img[x-2]*1 + img[x-1]*4 + img[x]*6 + img[x+1]*4 + img[x+2]*1;

// SIMD: 16 pixels at once
__m128i* img_vec = (__m128i*)img;
__m128i kernel = _mm_setr_epi8(1, 4, 6, 4, 1, ...);
for (x = 0; x < WIDTH; x += 16)
    output[x] = _mm_maddubs_epi16(img_vec[x], kernel);
```

**Expected speedup**: 4-8× for compute-bound kernels

### 2. Horizontal Tiling
Split image into horizontal strips and process on separate cores:

```
Core 0: Rows 0-1514     (with 2-pixel overlap)
Core 1: Rows 1515-3029
```

**Benefit**: Better load balancing (both cores finish simultaneously)

### 3. Combined Filter Kernel
Instead of separate Gauss → Sobel passes, fuse into single convolution:

```
Combined Kernel = Gauss_5x5 ⊗ Sobel_3x3 = 7×7 kernel
```

**Trade-off**: More complex kernel, but eliminates intermediate buffer

## Testing & Validation

### Unit Tests (Recommended)
1. **Median Filter**: Verify against known median values
2. **Gaussian**: Compare output with scipy.ndimage.gaussian_filter
3. **Sobel**: Validate against OpenCV Sobel implementation
4. **Ring Buffer**: Test state transitions under various scenarios

### Performance Benchmarking
1. Vary image sizes (512×512, 1024×1024, 4096×4096)
2. Profile individual functions with perf or gprof
3. Measure cache miss rates
4. Monitor memory bandwidth utilization

### Stress Testing
1. Run for extended periods (1M+ frames)
2. Monitor for memory leaks (Valgrind)
3. Verify determinism (execution time variance)
4. Test buffer starvation scenarios

## References

1. Canny Edge Detection: https://docs.opencv.org/3.4/da/d22/tutorial_py_canny.html
2. Sobel Operator: https://en.wikipedia.org/wiki/Sobel_operator
3. Separable Filters: https://www.songho.ca/dsp/convolution/convolution2d_separable.html
4. AMD Xilinx Zynq-7000: https://www.xilinx.com/products/silicon-devices/soc/zynq-7000.html

---

Last updated: January 2025
