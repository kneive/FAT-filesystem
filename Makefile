# ============================================================================
# FAT DRIVER MAKEFILE
# ============================================================================

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -Werror -std=c99 -Iinclude -g -O2
# -Wall -Wextra -Werror: Enable all warnings and treat them as errors
# -std=c99: Use C99 standard for portability
# -Iinclude: Add include/ directory to header search path
# -g: Include debug symbols for debugging
# -O2: Optimize for performance

# Directories
SRC_DIR = src
INC_DIR = include
TEST_DIR = tests
BUILD_DIR = build
OBJ_DIR = $(BUILD_DIR)/obj

# Output library
LIB = $(BUILD_DIR)/libfat.a

# Source files 
SRCS = $(wildcard $(SRC_DIR)/*.c)
# Example: SRCS = $(SRC_DIR)/fat_boot.c $(SRC_DIR)/fat_volume.c

# Object files
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

# Test files 
TEST_SRCS = $(wildcard $(TEST_DIR)/*.c)
TEST_OBJS = $(TEST_SRCS:$(TEST_DIR)/%.c=$(OBJ_DIR)/test_%.o)
TEST_BIN = $(BUILD_DIR)/test_fat

# ============================================================================
# TARGETS
# ============================================================================

# Default target: build everything
all: $(LIB) driver

# Build the static library
# WHY STATIC LIBRARY: Easy to link into other projects
$(LIB): $(OBJS)
	@mkdir -p $(BUILD_DIR)
	ar rcs $@ $^
	@echo "Built library: $@"

# Compile source files to object files
# PATTERN RULE: Automatically compiles any .c file in src/ to .o in build/obj/
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Compile test files
$(OBJ_DIR)/test_%.o: $(TEST_DIR)/%.c
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Build and run tests
test: $(TEST_BIN)
	./$(TEST_BIN)

$(TEST_BIN): $(TEST_OBJS) $(LIB)
	$(CC) $(CFLAGS) $^ -o $@

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR)

# Create directory structure
# PURPOSE: Initialize project directories
dirs:
	mkdir -p $(SRC_DIR) $(INC_DIR) $(TEST_DIR) $(BUILD_DIR) $(OBJ_DIR)

# Show help
help:
	@echo "FAT Driver Build System"
	@echo ""
	@echo "Targets:"
	@echo "  all    - Build the FAT driver library (default)"
	@echo "  test   - Build and run tests"
	@echo "  clean  - Remove all build artifacts"
	@echo "  dirs   - Create project directory structure"
	@echo "  help   - Show this help message"

# executable
DRIVER = $(BUILD_DIR)/driver

# build
driver: $(LIB)
		$(CC) $(CFLAGS) $(SRC_DIR)/driver.c -o $(DRIVER) -L$(BUILD_DIR) -lfat
		@echo "Build driver: $(DRIVER)"

.PHONY: all test clean dirs help driver