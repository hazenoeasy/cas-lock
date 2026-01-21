# Makefile for CAS Lock Library
# ARM Atomic Lock Library

# Compiler settings - detect host architecture
UNAME_S := $(shell uname -s)
UNAME_M := $(shell uname -m)

ifeq ($(UNAME_M),x86_64)
    ARCH_FLAGS = -arch x86_64
else ifeq ($(UNAME_M),arm64)
    ARCH_FLAGS = -arch arm64
else
    ARCH_FLAGS =
endif

CC = gcc
CFLAGS = -Wall -Wextra -O2 $(ARCH_FLAGS) -I./include
LDFLAGS = -pthread

# Directories
SRC_DIR = src
INC_DIR = include
TEST_DIR = tests
BUILD_DIR = build

# Source files (currently all headers with inline functions)
# Future: add .c files here

# Test targets
TEST_TARGET = $(BUILD_DIR)/test_locks
BENCH_TARGET = $(BUILD_DIR)/bench_locks

# Default target
.PHONY: all
all: $(BUILD_DIR) $(TEST_TARGET) $(BENCH_TARGET)

# Create build directory
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

# Build correctness test
$(TEST_TARGET): $(TEST_DIR)/test_locks.c
	@echo "Building correctness test..."
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<
	@echo "  -> $@"

# Build benchmark
$(BENCH_TARGET): $(TEST_DIR)/bench_locks.c
	@echo "Building benchmark..."
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<
	@echo "  -> $@"

# Run correctness tests
.PHONY: test
test: $(TEST_TARGET)
	@echo ""
	@echo "Running correctness tests..."
	@echo ""
	@$(TEST_TARGET)

# Run benchmarks
.PHONY: bench
bench: $(BENCH_TARGET)
	@echo ""
	@echo "Running benchmarks..."
	@echo ""
	@$(BENCH_TARGET)

# Run all tests
.PHONY: check
check: test bench

# Clean build artifacts
.PHONY: clean
clean:
	@echo "Cleaning build artifacts..."
	@rm -rf $(BUILD_DIR)
	@echo "Done."

# Display help
.PHONY: help
help:
	@echo "CAS Lock Library - Build System"
	@echo ""
	@echo "Available targets:"
	@echo "  all      - Build all targets (default)"
	@echo "  test     - Build and run correctness tests"
	@echo "  bench    - Build and run benchmarks"
	@echo "  check    - Run all tests (correctness + benchmark)"
	@echo "  clean    - Remove build artifacts"
	@echo "  help     - Display this help message"
	@echo ""
	@echo "Note: This library is designed for ARM architecture."
	@echo "      For cross-compilation, set CC and CFLAGS appropriately."
