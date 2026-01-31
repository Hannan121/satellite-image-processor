# Simple Compilation Instructions (GCC Only)

## Windows (MinGW/MSYS2)

### Single-Core Version
```bash
gcc src/main.c src/tasks_v2.c -o satellite_sim.exe -O3
```

### Dual-Core Version
```bash
gcc src/main_dualcore.c src/tasks_v2.c -o satellite_dualcore_sim.exe -O3
```

That's it!

## Why -O3?

- **Compiler handles optimizations**: Loop unrolling, function inlining, auto-vectorization
- **Portable**: No architecture-specific instructions (SSE/AVX/NEON)
- **Deterministic**: Behavior consistent across different CPUs
- **Good enough**: Gets you 35ms/22ms performance without manual SIMD

## Run the programs

```bash
# Generate test data (creates input_image.bin)
python scripts/generate_test_image.py

# Run single-core
./satellite_sim.exe

# Run dual-core
./satellite_dualcore_sim.exe

# Visualize output
python scripts/view_output.py
```

## Optional: Try different optimization levels

```bash
# No optimization (for debugging)
gcc src/main.c src/tasks_v2.c -o satellite_sim.exe -O0

# Basic optimization
gcc src/main.c src/tasks_v2.c -o satellite_sim.exe -O2

# Aggressive optimization (your current choice)
gcc src/main.c src/tasks_v2.c -o satellite_sim.exe -O3
```

-O3 is the right balance for embedded work: fast, portable, predictable.

---

**Note**: The Makefile is included for convenience, but you don't need it. These gcc commands are all you need.
