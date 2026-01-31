# Quick Start Guide

## Testing Without Original Image

If you don't have the original test image, you can generate a synthetic star field:

### Generate Synthetic Test Image

```bash
python scripts/generate_test_image.py
```

This creates **both**:
- `Raw_image_Fig1.jpg` - Visual reference image
- `input_image.bin` - Binary file ready for C program

**No conversion step needed!** You can immediately run:
```bash
./satellite_sim.exe       # or make run-single
```

### Manual Conversion (if you have your own JPG)

If you have a different 3124×3030 test image:

### Manual Conversion (if you have your own JPG)

If you have a different 3124×3030 test image:

```python
# convert_jpg_to_bin.py
from PIL import Image

img = Image.open('your_image.jpg').convert('L')
with open('input_image.bin', 'wb') as f:
    f.write(img.tobytes())
```

Or use the provided script:
```bash
python scripts/convert_jpg_to_bin.py
```

## Windows Compilation

### Using MinGW-w64
```bash
gcc main.c tasks_v2.c -o satellite_sim.exe -O3
```

### Using MSVC (Visual Studio)
```cmd
cl /O2 /Fe:satellite_sim.exe main.c tasks_v2.c
```

## Linux Compilation (with modifications)

The current code uses Windows-specific APIs. For Linux, modify timing and threading:

```c
// Replace Windows.h timing with:
#include <time.h>
double get_time_ms() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1000000.0;
}

// Replace _beginthreadex with:
#include <pthread.h>
pthread_create(&thread, NULL, Core0_Thread, NULL);

// Replace SetThreadAffinityMask with:
cpu_set_t cpuset;
CPU_ZERO(&cpuset);
CPU_SET(0, &cpuset);
pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);

// Replace Sleep() with:
#include <unistd.h>
usleep(1000);  // Sleep for 1ms
```

Then compile:
```bash
gcc main.c tasks_v2.c -o satellite_sim -O3 -pthread
```

## Expected Output

### Console Output (Single-Core)
```
System initialization...
[SYS] Loaded test vector: input_image.bin (9465720 bytes)
commencing single core simulation
[DEBUG] Dumping Frame 0 inference & Image...
Simulation complete 
Execution Performance metrics:
Frame id  , Total execution time, Median time, edge detection time 
 0        , 46.810500          , 11.863100       , 34.947400  
 1        , 35.173400          , 12.285400       , 22.888000  
 ...
```

### Output Files
- `output_frame0.pgm` - Grayscale edge image (viewable with GIMP, ImageJ, or view_output.py)
- `output_inference0.csv` - List of edge pixels:
  ```
  Address,Value
  9467844,127
  9467845,142
  ...
  ```

## Visualization

```bash
python view_output.py
```

This will:
1. Convert PGM to PNG
2. Display the image using your default image viewer
3. Save as `output_visualized.png`

## Performance Tuning

### Compiler Optimization Flags

Try different optimization levels:
```bash
# Basic optimization
gcc main.c tasks_v2.c -o satellite_sim -O2

# Aggressive optimization
gcc main.c tasks_v2.c -o satellite_sim -O3 -march=native -mtune=native

# With profile-guided optimization (PGO)
gcc main.c tasks_v2.c -o satellite_sim -O3 -fprofile-generate
./satellite_sim.exe
gcc main.c tasks_v2.c -o satellite_sim -O3 -fprofile-use
```

### Configuration Tuning

Edit `system_def.h`:

**Reduce memory usage:**
```c
#define BUFF_SIZE 2  // Minimum for double-buffering
```

**Adjust edge sensitivity:**
```c
#define EDGE_THRESHOLD 20  // Lower = more edges detected
#define EDGE_THRESHOLD 60  // Higher = fewer edges, only strong edges
```

**Change ROI size:**
```c
#define MEDIAN_SUBFRAME_SIZE 50   // Faster, less accurate background removal
#define MEDIAN_SUBFRAME_SIZE 200  // Slower, more accurate background removal
```

## Troubleshooting

### "Could not open input_image.bin"
→ Run `python convert_jpg_to_bin.py` first

### "File size mismatch"
→ Ensure your test image is exactly 3124×3030 pixels

### Poor edge detection results
→ Adjust `EDGE_THRESHOLD` in `system_def.h`
→ Try different test images with more contrast

### Compilation errors on Linux
→ Replace Windows-specific code as shown above
→ Add `-pthread` flag to gcc command

### Dual-core version shows no speedup
→ Check Task Manager/htop to verify both cores are active
→ Ensure thread affinity is set correctly
→ Monitor for buffer starvation errors

## Next Steps

1. Try different test images (synthetic or real)
2. Experiment with different threshold values
3. Profile the code to find bottlenecks
4. Implement SIMD optimizations
5. Port to Linux/POSIX for cross-platform support

## Support

For questions or issues, open an issue on GitHub or contact the author.
