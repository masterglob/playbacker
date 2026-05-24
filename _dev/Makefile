# ==============================================================================
# PLAYBACKER MAKEFILE (Modular Architecture)
# Target: Raspberry Pi 3B (piCore / TinyCore)
# ==============================================================================

# 1. DIRECTORIES
SRC_DIR   = src
LIB_DIR   = $(SRC_DIR)/pbkr_lib
MAINS_DIR = $(SRC_DIR)/mains
OBJ_DIR   = obj
OUT_DIR   = out

# 2. COMPILER & LINKER CONFIGURATION
CC  = gcc
CXX = g++
AR  = /usr/local/bin/ar

# Optimizations for Raspberry Pi 3B (Cortex-A53)
COMMON_FLAGS = -Wall -O1 -ggdb -mcpu=cortex-a53 -mfpu=neon-fp-armv8 -mfloat-abi=hard -I$(LIB_DIR) -MMD -MP

ifneq ($(BUILD_ID),)
    COMMON_FLAGS += -DBUILD_ID=$(BUILD_ID)
    FORCE_TOUCH := $(shell touch $(LIB_DIR)/pbkr_version.cpp)
endif

CFLAGS   = $(COMMON_FLAGS) -std=gnu99
CXXFLAGS = $(COMMON_FLAGS) -std=c++14

# Libraries required for linking the final binary
LIBS    = -lasound -lwiringPi -lusb-1.0 -lpthread
LDFLAGS = $(LIBS)

# 3. STATIC LIBRARY (pbkr_lib) CONFIGURATION
STATIC_LIB = $(OBJ_DIR)/libpbkr.a

# Find all sources inside pbkr_lib
LIB_SRCS_CPP := $(shell find $(LIB_DIR) -name "*.cpp")
LIB_SRCS_C   := $(shell find $(LIB_DIR) -name "*.c")

# Map to object files inside obj/pbkr_lib/
LIB_OBJS := $(patsubst $(LIB_DIR)/%.cpp, $(OBJ_DIR)/pbkr_lib/%.o, $(LIB_SRCS_CPP)) \
            $(patsubst $(LIB_DIR)/%.c, $(OBJ_DIR)/pbkr_lib/%.o, $(LIB_SRCS_C))

# 4. EXECUTABLES (mains) CONFIGURATION
MAIN_SRCS := $(shell find $(MAINS_DIR) -name "*.cpp")

# Generate target paths in out/ (e.g., src/mains/pbkr.cpp -> out/pbkr)
TARGETS := $(patsubst $(MAINS_DIR)/%.cpp, $(OUT_DIR)/%, $(MAIN_SRCS))

ALL_DEPS := $(LIB_OBJS:.o=.d) $(patsubst $(MAINS_DIR)/%.cpp, $(OBJ_DIR)/mains/%.d, $(MAIN_SRCS))

# ==============================================================================
# BUILD RULES
# ==============================================================================

.PHONY: all clean info

# Default target
all: $(STATIC_LIB) $(TARGETS)

-include $(ALL_DEPS)

# --- RULE 1: BUILD THE STATIC LIBRARY ---
$(STATIC_LIB): $(LIB_OBJS)
	@echo "=================================================="
	@echo "📦 Creating Static Library: $@"
	@echo "=================================================="
	@mkdir -p $(dir $@)
	$(AR) rcs $@ $(LIB_OBJS)

# Compiling C++ files for the library
$(OBJ_DIR)/pbkr_lib/%.o: $(LIB_DIR)/%.cpp
	@mkdir -p $(dir $@)
	@echo "🛠️  [Lib] Compiling C++: $<"
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Compiling C files for the library
$(OBJ_DIR)/pbkr_lib/%.o: $(LIB_DIR)/%.c
	@mkdir -p $(dir $@)
	@echo "🛠️  [Lib] Compiling C: $<"
	$(CC) $(CFLAGS) -c $< -o $@


# --- RULE 2: BUILD EXECUTABLES FROM MAINS ---
$(OUT_DIR)/%: $(MAINS_DIR)/%.cpp $(STATIC_LIB)
	@mkdir -p $(dir $@)
	@echo "=================================================="
	@echo "🔗 Linking Main Executable: $@"
	@echo "=================================================="
	$(CXX) $(CXXFLAGS) $< $(STATIC_LIB) -o $@ $(LDFLAGS)
	@echo "🚀 Build Complete: $@"


# --- UTILITY RULES ---
clean:
	@echo "🧹 Cleaning up build artifacts..."
	rm -rf $(OBJ_DIR) $(OUT_DIR)
	@echo "✨ Clean finished."

info:
	@echo "=== Playbacker Project Structure ==="
	@echo "Lib Sources (C++): $(LIB_SRCS_CPP)"
	@echo "Lib Objects:       $(LIB_OBJS)"
	@echo "Main Sources:      $(MAIN_SRCS)"
	@echo "Targets Generated: $(TARGETS)"
	@echo "===================================="