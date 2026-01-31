# Satellite Image Processor Makefile
# Supports Windows (MinGW) and Linux (GCC)

# Compiler
CC = gcc

# Compiler flags
CFLAGS = -O3 -Wall -Wextra
LDFLAGS = 

# Detect OS
ifeq ($(OS),Windows_NT)
    # Windows-specific
    EXE_EXT = .exe
    LDFLAGS += -lwinmm
    RM = del /Q
    MKDIR = mkdir
else
    # Linux-specific (requires code modifications for threading)
    EXE_EXT = 
    LDFLAGS += -pthread
    RM = rm -f
    MKDIR = mkdir -p
endif

# Source files
SRC_DIR = src
SCRIPT_DIR = scripts

SINGLE_CORE_SRC = $(SRC_DIR)/main.c $(SRC_DIR)/tasks_v2.c
DUAL_CORE_SRC = $(SRC_DIR)/main_dualcore.c $(SRC_DIR)/tasks_v2.c

# Output binaries
SINGLE_CORE_BIN = satellite_sim$(EXE_EXT)
DUAL_CORE_BIN = satellite_dualcore_sim$(EXE_EXT)

# Default target
.PHONY: all
all: single dual

# Single-core version
.PHONY: single
single: $(SINGLE_CORE_BIN)

$(SINGLE_CORE_BIN): $(SINGLE_CORE_SRC)
	$(CC) $(CFLAGS) $(SINGLE_CORE_SRC) -o $(SINGLE_CORE_BIN) $(LDFLAGS)
	@echo "Built single-core version: $(SINGLE_CORE_BIN)"

# Dual-core version
.PHONY: dual
dual: $(DUAL_CORE_BIN)

$(DUAL_CORE_BIN): $(DUAL_CORE_SRC)
	$(CC) $(CFLAGS) $(DUAL_CORE_SRC) -o $(DUAL_CORE_BIN) $(LDFLAGS)
	@echo "Built dual-core version: $(DUAL_CORE_BIN)"

# Generate synthetic test image
.PHONY: testimage
testimage:
	python $(SCRIPT_DIR)/generate_test_image.py

# Convert JPG to binary
.PHONY: convert
convert:
	python $(SCRIPT_DIR)/convert_jpg_to_bin.py

# Run single-core simulation
.PHONY: run-single
run-single: single
	./$(SINGLE_CORE_BIN)

# Run dual-core simulation
.PHONY: run-dual
run-dual: dual
	./$(DUAL_CORE_BIN)

# Visualize output
.PHONY: visualize
visualize:
	python $(SCRIPT_DIR)/view_output.py

# Complete workflow (generate → compile → run → visualize)
.PHONY: demo
demo: testimage single run-single visualize
	@echo "Demo complete! Check output_visualized.png"

# Clean generated files
.PHONY: clean
clean:
	$(RM) $(SINGLE_CORE_BIN) $(DUAL_CORE_BIN)
	$(RM) *.o *.bin *.pgm *.csv output_*.png
	@echo "Cleaned build artifacts"

# Clean everything including test images
.PHONY: distclean
distclean: clean
	$(RM) Raw_image_Fig1.jpg input_image.bin
	@echo "Cleaned all generated files"

# Help
.PHONY: help
help:
	@echo "Satellite Image Processor - Makefile targets:"
	@echo ""
	@echo "  make all          - Build both single and dual core versions"
	@echo "  make single       - Build single-core version only"
	@echo "  make dual         - Build dual-core version only"
	@echo "  make testimage    - Generate synthetic test image (creates .bin automatically)"
	@echo "  make convert      - Convert JPG to binary format (for custom images)"
	@echo "  make run-single   - Compile and run single-core simulation"
	@echo "  make run-dual     - Compile and run dual-core simulation"
	@echo "  make visualize    - Display output image"
	@echo "  make demo         - Complete workflow (generate → compile → run → visualize)"
	@echo "  make clean        - Remove compiled binaries and outputs"
	@echo "  make distclean    - Remove all generated files including test images"
	@echo "  make help         - Show this help message"
	@echo ""
	@echo "Quick start:"
	@echo "  make demo         # Does everything automatically!"
	@echo ""
	@echo "Manual workflow:"
	@echo "  1. make testimage       # Generate synthetic star field + .bin file"
	@echo "  2. make single          # Compile single-core version"
	@echo "  3. ./satellite_sim.exe  # Run simulation"
	@echo "  4. make visualize       # View results"
