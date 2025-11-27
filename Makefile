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

# HLFFI wrapper sources (implemented files)
HLFFI_WRAPPER_SRC = \
	src/hlffi_core.c \
	src/hlffi_lifecycle.c \
	src/hlffi_types.c \
	src/hlffi_values.c

# Stub files (not yet implemented, excluded from Linux build):
# src/hlffi_events.c
# src/hlffi_integration.c
# src/hlffi_reload.c
# src/hlffi_threading.c

# HashLink VM core sources (EXCLUDING allocator.c which must be with gc.c)
HL_VM_SRC = \
	vendor/hashlink/src/code.c \
	vendor/hashlink/src/module.c \
	vendor/hashlink/src/jit.c \
	vendor/hashlink/src/debugger.c \
	vendor/hashlink/src/profile.c

# HashLink libhl sources (GC + stdlib + PCRE2)
# NOTE: gc.c includes allocator.c, so we don't compile allocator.c separately
LIBHL_SRC = \
	vendor/hashlink/src/gc.c \
	$(wildcard vendor/hashlink/src/std/*.c) \
	$(wildcard vendor/hashlink/include/pcre/*.c)

# Remove files that should not be compiled directly (included by other files)
# - PCRE2 JIT files are included by pcre2_jit_compile.c
# - allocator.c is included by gc.c
LIBHL_SRC := $(filter-out %/pcre2_jit_match.c %/pcre2_jit_test.c %/pcre2_jit_misc.c %/allocator.c, $(LIBHL_SRC))

# Plugin wrappers (disabled for Linux - libuv/mbedtls are Windows-only in this vendored version)
# PLUGIN_SRC = \
#	vendor/hashlink/libs/uv/uv.c \
#	vendor/hashlink/libs/ssl/ssl.c
PLUGIN_SRC =

# libuv sources (disabled - only Windows platform code available)
# LIBUV_SRC = \
#	$(wildcard vendor/hashlink/include/libuv/src/*.c) \
#	$(wildcard vendor/hashlink/include/libuv/src/unix/*.c)
# Filter out Windows-specific libuv files
# LIBUV_SRC := $(filter-out %/win/%, $(LIBUV_SRC))
LIBUV_SRC =

# mbedtls sources (disabled for Linux build)
# MBEDTLS_SRC = $(wildcard vendor/hashlink/include/mbedtls/library/*.c)
MBEDTLS_SRC =

# hlffi.a sources (wrapper + VM core only for Linux)
# NOTE: For Windows, plugins and embedded deps will be included
HLFFI_SRC = $(HLFFI_WRAPPER_SRC) $(HL_VM_SRC) $(PLUGIN_SRC) $(LIBUV_SRC) $(MBEDTLS_SRC)

# Object files
LIBHL_OBJ = $(LIBHL_SRC:%.c=$(BUILD_DIR)/%.o)
HLFFI_OBJ = $(HLFFI_SRC:%.c=$(BUILD_DIR)/%.o)

# Output libraries
LIBHL = $(BIN_DIR)/libhl.a
HLFFI = $(BIN_DIR)/libhlffi.a

# Test executables
TEST_LIBHL = test_libhl
TEST_HELLO = test_hello
TEST_RUNNER = test_runner
TEST_REFLECTION = test_reflection
TEST_STATIC = test_static

# Linker flags for tests
# CRITICAL: Must use --whole-archive for libhl.a to expose all primitives to dlsym()
# and -rdynamic to export symbols from executable for RTLD_DEFAULT lookups
LDFLAGS = -Lbin -Wl,--whole-archive -lhl -Wl,--no-whole-archive -ldl -lm -lpthread -rdynamic

.PHONY: all clean libhl hlffi info tests

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
	rm -f $(TEST_LIBHL) $(TEST_HELLO) $(TEST_RUNNER) $(TEST_REFLECTION)
	@echo "Cleaned build artifacts"

# Print detailed info
verbose: info
	@echo "LIBHL sources ($(words $(LIBHL_SRC)) files):"
	@for src in $(LIBHL_SRC); do echo "  - $$src"; done
	@echo ""
	@echo "HLFFI sources ($(words $(HLFFI_SRC)) files):"
	@for src in $(HLFFI_SRC); do echo "  - $$src"; done

# Test targets
tests: $(TEST_LIBHL) $(TEST_HELLO) $(TEST_RUNNER) $(TEST_REFLECTION) $(TEST_STATIC)
	@echo ""
	@echo "✓ All tests built successfully!"
	@echo ""
	@echo "Run tests:"
	@echo "  ./$(TEST_LIBHL)                  - Test basic libhl.a linking"
	@echo "  ./$(TEST_HELLO) test/hello.hl    - Test direct HashLink API"
	@echo "  ./$(TEST_RUNNER) test/hello.hl   - Test HLFFI wrapper API"
	@echo "  ./$(TEST_REFLECTION) test/hello.hl - Test Phase 2 type reflection"
	@echo "  ./$(TEST_STATIC) test/static_test.hl - Test Phase 3 static members & values"
	@echo ""

$(TEST_LIBHL): test_libhl.c $(LIBHL)
	@echo "Building $@..."
	$(CC) -o $@ $< -Ivendor/hashlink/src $(LDFLAGS)

$(TEST_HELLO): test_hello.c $(LIBHL) $(HLFFI)
	@echo "Building $@..."
	$(CC) -o $@ $< -Ivendor/hashlink/src -Lbin -lhlffi $(LDFLAGS)

$(TEST_RUNNER): test_runner.c $(LIBHL) $(HLFFI)
	@echo "Building $@..."
	$(CC) -o $@ $< -Iinclude -Ivendor/hashlink/src -Lbin -lhlffi $(LDFLAGS)

$(TEST_REFLECTION): test_reflection.c $(LIBHL) $(HLFFI)
	@echo "Building $@..."
	$(CC) -o $@ $< -Iinclude -Ivendor/hashlink/src -Lbin -lhlffi $(LDFLAGS)

$(TEST_STATIC): test_static.c $(LIBHL) $(HLFFI)
	@echo "Building $@..."
	$(CC) -o $@ $< -Iinclude -Ivendor/hashlink/src -Lbin -lhlffi $(LDFLAGS)
