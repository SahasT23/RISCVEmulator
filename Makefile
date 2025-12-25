# RISC-V Emulator Makefile

CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -g
INCLUDES = -Iinclude

SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin

# Source files
SOURCES = $(wildcard $(SRC_DIR)/*.cpp)
OBJECTS = $(SOURCES:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)

# Target executable
TARGET = $(BIN_DIR)/riscv-emu

# Default target
all: directories $(TARGET)

# Create directories
directories:
	@mkdir -p $(OBJ_DIR) $(BIN_DIR)

# Link
$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Compile
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c -o $@ $<

# Clean
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

# Run
run: all
	./$(TARGET)

# Run with a file
run-file: all
	./$(TARGET) $(FILE)

# Debug build
debug: CXXFLAGS = -std=c++17 -Wall -Wextra -g -O0 -DDEBUG
debug: clean all

# Dependencies
$(OBJ_DIR)/main.o: include/emulator.hpp
$(OBJ_DIR)/emulator.o: include/emulator.hpp include/common.hpp include/memory.hpp include/register_file.hpp include/assembler.hpp include/cpu.hpp include/pipeline.hpp include/decoder.hpp
$(OBJ_DIR)/cpu.o: include/cpu.hpp include/common.hpp include/memory.hpp include/register_file.hpp include/alu.hpp include/decoder.hpp
$(OBJ_DIR)/pipeline.o: include/pipeline.hpp include/common.hpp include/memory.hpp include/register_file.hpp include/alu.hpp include/decoder.hpp include/hazard_unit.hpp
$(OBJ_DIR)/hazard_unit.o: include/hazard_unit.hpp include/common.hpp include/decoder.hpp
$(OBJ_DIR)/assembler.o: include/assembler.hpp include/common.hpp include/memory.hpp
$(OBJ_DIR)/decoder.o: include/decoder.hpp include/common.hpp
$(OBJ_DIR)/alu.o: include/alu.hpp include/common.hpp
$(OBJ_DIR)/memory.o: include/memory.hpp include/common.hpp
$(OBJ_DIR)/register_file.o: include/register_file.hpp include/common.hpp

.PHONY: all clean run run-file debug directories
