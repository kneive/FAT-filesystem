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
LDFLAGS =

# Directories
SRC_DIR = src
INC_DIR = include
TEST_DIR = tests
BUILD_DIR = build
OBJ_DIR = $(BUILD_DIR)/obj
TEST_OBJ_DIR = $(BUILD_DIR)/test_obj

# Unity
UNITY_DIR = $(TEST_DIR)/unity
UNITY_SRC = $(UNITY_DIR)/unity.c
UNITY_OBJ = $(TEST_OBJ_DIR)/unity.o

# Output library
LIB = $(BUILD_DIR)/libfat.a

# Source files 
SRCS = $(filter-out $(SRC_DIR)/driver.c, $(wildcard $(SRC_DIR)/*.c))

# Object files
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

# Test files 
TEST_SRCS = $(wildcard $(TEST_DIR)/unit/*.c)
TEST_OBJS = $(TEST_SRCS:$(TEST_DIR)/unit/%.c=$(BUILD_DIR)/%)

DRIVER = $(BUILD_DIR)/driver

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

# Compile unity framework
$(UNITY_OBJ): $(UNITY_SRC)
	@mkdir -p $(TEST_OBJ_DIR)
	$(CC) $(CFLAGS) -I$(UNITY_DIR) -c $< -o $@

# Compile individual test files and link them
$(BUILD_DIR)/test_%: $(TEST_DIR)/unit/test_%.c $(UNITY_OBJ) $(LIB)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(UNITY_DIR) $^ -o $@ $(LDFFLAGS)
	@echo "Build test: $@"

# Build all tests
build-tests: $(TEST_BINS)
	@echo "All tests built successfully"

# Build and run tests
test: build-tests
		@echo "Running tests"
		@for test in $(TEST_BINS); DO \
			echo ""; \
			echo "========================================"; \
			echo "Running $$test"; \
			echo "========================================"; \
			./$$test || exit 1; \
		done
		@echo ""
		@echo "All tests passed!"

# run specific test
test-one: $(BUILD_DIR)/$(TEST)
		@echo "Running $(TEST)..."
		./$(BUILD_DIR)/$(TEST)

# build driver
driver: $(LIB)
		$(CC) $(CFLAGS) $(SRC_DIR)/driver.c -o $(DRIVER) -L$(BUILD_DIR) -lfat
		@echo "Build driver: $(DRIVER)"

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR)

# Create directory structure
# PURPOSE: Initialize project directories
dirs:
	mkdir -p $(SRC_DIR) $(INC_DIR) $(BUILD_DIR) $(OBJ_DIR) $(TEST_OBJ_DIR) 
	mkdir -p $(TEST_DIR)/unit $(TEST_DIR)/integration $(TEST_DIR)/helpers $(UNITY_DIR)

# Show help
help:
	@echo "FAT Driver Build System"
	@echo ""
	@echo "Targets:"
	@echo "  all          - Build the FAT driver library and driver (default)"
	@echo "  driver       - Build the driver executable"
	@echo "  build-tests  - Build all test executables"
	@echo "  test         - Build and run all tests"
	@echo "  test-one     - Run a specific test (usage: make test-one TEST=test_fat_block_device)"
	@echo "  clean        - Remove all build artifacts"
	@echo "  dirs         - Create project directory structure"
	@echo "  help         - Show this help message"
	@echo ""
	@echo "Examples:"
	@echo "  make test                                    # Run all tests"
	@echo "  make test-one TEST=test_fat_block_device     # Run one test"

.PHONY: all test test-one build-tests clean dirs help driver