 ############## LLM Generated Code Begins ##############

# Compiler and flags
CC = gcc
CFLAGS = -std=c99 \
         -D_POSIX_C_SOURCE=200809L \
         -D_XOPEN_SOURCE=700 \
         -Wall -Wextra -Werror \
         -Wno-unused-parameter \
         -fno-asm
LDFLAGS =

# Directories
SRC_DIR = src
INCLUDE_DIR = include
BUILD_DIR = obj
# The TARGET is now just the filename
TARGET = shell.out

# Source files and object files
SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS))

# Header files dependency
DEPS := $(wildcard $(INCLUDE_DIR)/*.h)

# Default target
all: $(TARGET)

# Link the object files to create the final executable
$(TARGET): $(OBJS)
	# No more mkdir command needed
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "Shell compiled successfully to $(TARGET)"

# Compile source files into object files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c $(DEPS)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(INCLUDE_DIR) -c $< -o $@

# Clean up build files
clean:
	rm -rf $(BUILD_DIR)
	rm -f $(TARGET)

.PHONY: all clean

 ############## LLM Generated Code Ends ##############