# HLFFI Linux Makefile (for testing compilation on x86_64 Linux)
# This validates that all sources compile correctly

CC = gcc
AR = ar
CFLAGS = -Wall -O2 -fPIC -std=gnu11
DEFINES = -DLIBHL_EXPORTS -DHAVE_CONFIG_H -DPCRE2_CODE_UNIT_WIDTH=16

# Include directories
INCLUDES = -Iinclude \
           -Ivendor/hashlink/src \
           -Ivendor/hashlink/include/pcre \
           -Ivendor/hashlink/include/libuv/include \
           -Ivendor/hashlink/include/libuv/src \
           -Ivendor/hashlink/include/mbedtls/include \
           -Ivendor/hashlink/libs/ssl

# Disable specific warnings (matching Visual Studio settings)
CFLAGS += -Wno-unused-parameter -Wno-unused-variable -Wno-sign-compare

# Output directories
BUILD_DIR = build
BIN_DIR = bin

# HLFFI wrapper sources (only implemented files for now)
HLFFI_WRAPPER_SRC = \
	src/hlffi_core.c \
	src/hlffi_lifecycle.c

# Stub files (not yet implemented, excluded from Linux build):
# src/hlffi_events.c
# src/hlffi_integration.c
# src/hlffi_reload.c
# src/hlffi_threading.c

# HashLink VM core sources
HL_VM_SRC = \
	vendor/hashlink/src/allocator.c \
	vendor/hashlink/src/code.c \
	vendor/hashlink/src/module.c \
	vendor/hashlink/src/jit.c \
	vendor/hashlink/src/debugger.c \
	vendor/hashlink/src/profile.c

# HashLink libhl sources (GC + stdlib + PCRE2)
LIBHL_SRC = \
	vendor/hashlink/src/gc.c \
	$(wildcard vendor/hashlink/src/std/*.c) \
	$(wildcard vendor/hashlink/include/pcre/*.c)

# Remove files that should not be compiled directly (included by other files)
LIBHL_SRC := $(filter-out %/pcre2_jit_match.c %/pcre2_jit_test.c %/pcre2_jit_misc.c, $(LIBHL_SRC))

# Plugin wrappers
PLUGIN_SRC = \
	vendor/hashlink/libs/uv/uv.c \
	vendor/hashlink/libs/ssl/ssl.c

# libuv sources
LIBUV_SRC = \
	$(wildcard vendor/hashlink/include/libuv/src/*.c) \
	$(wildcard vendor/hashlink/include/libuv/src/unix/*.c)

# Filter out Windows-specific libuv files
LIBUV_SRC := $(filter-out %/win/%, $(LIBUV_SRC))

# mbedtls sources
MBEDTLS_SRC = $(wildcard vendor/hashlink/include/mbedtls/library/*.c)

# hlffi.a sources (wrapper + VM core + plugins + embedded deps)
HLFFI_SRC = $(HLFFI_WRAPPER_SRC) $(HL_VM_SRC) $(PLUGIN_SRC) $(LIBUV_SRC) $(MBEDTLS_SRC)

# Object files
LIBHL_OBJ = $(LIBHL_SRC:%.c=$(BUILD_DIR)/%.o)
HLFFI_OBJ = $(HLFFI_SRC:%.c=$(BUILD_DIR)/%.o)

# Output libraries
LIBHL = $(BIN_DIR)/libhl.a
HLFFI = $(BIN_DIR)/libhlffi.a

.PHONY: all clean libhl hlffi info

all: info $(LIBHL) $(HLFFI)
	@echo ""
	@echo "✓ Build complete!"
	@echo "  libhl.a:   $(shell wc -c < $(LIBHL)) bytes"
	@echo "  libhlffi.a: $(shell wc -c < $(HLFFI)) bytes"
	@echo ""
	@echo "Libraries built successfully. Both static libraries compile without errors."

info:
	@echo "Building HLFFI v3.0 static libraries..."
	@echo ""
	@echo "libhl.a:    $(words $(LIBHL_SRC)) source files"
	@echo "libhlffi.a: $(words $(HLFFI_SRC)) source files"
	@echo ""

libhl: $(LIBHL)

hlffi: $(HLFFI)

$(LIBHL): $(LIBHL_OBJ) | $(BIN_DIR)
	@echo "Creating $@..."
	$(AR) rcs $@ $^

$(HLFFI): $(HLFFI_OBJ) | $(BIN_DIR)
	@echo "Creating $@..."
	$(AR) rcs $@ $^

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -c $< -o $@

$(BIN_DIR):
	@mkdir -p $(BIN_DIR)

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)
	@echo "Cleaned build artifacts"

# Print detailed info
verbose: info
	@echo "LIBHL sources ($(words $(LIBHL_SRC)) files):"
	@for src in $(LIBHL_SRC); do echo "  - $$src"; done
	@echo ""
	@echo "HLFFI sources ($(words $(HLFFI_SRC)) files):"
	@for src in $(HLFFI_SRC); do echo "  - $$src"; done
