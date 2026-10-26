# ============================================================
# Makefile — Packet Routing Simulator
# Target: Windows + MSYS2 UCRT64 GCC
# Usage:
#   mingw32-make          # build simulator
#   mingw32-make test     # build and run tests
#   mingw32-make clean    # remove build artefacts
#   mingw32-make debug    # build with AddressSanitizer (if available)
# ============================================================

CC      = C:/msys64/ucrt64/bin/gcc.exe
CFLAGS  = -std=c11 -Wall -Wextra -Wpedantic -Wshadow \
          -Wno-unused-parameter -g
LDFLAGS = -lm

# Source files
SRCS = main.c \
       src/router.c \
       src/network.c \
       src/routing.c \
       src/packet.c \
       src/simulation.c \
       src/statistics.c \
       src/input_util.c

# Test sources (excludes main.c to avoid duplicate main)
TEST_SRCS = tests/tests.c \
            src/router.c \
            src/network.c \
            src/routing.c \
            src/packet.c \
            src/statistics.c \
            src/input_util.c

TARGET     = simulator.exe
TEST_TARGET = tests/run_tests.exe

# ── default target ──────────────────────────────────────────
all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo Built: $(TARGET)

# ── test target ─────────────────────────────────────────────
test: $(TEST_TARGET)
	@echo Running tests...
	./$(TEST_TARGET)

$(TEST_TARGET): $(TEST_SRCS)
	$(CC) $(CFLAGS) -I. -o $@ $^ $(LDFLAGS)
	@echo Built: $(TEST_TARGET)

# ── debug build with AddressSanitizer ──────────────────────
# Note: ASan may not be available on all MSYS2 installations.
# If this fails, remove -fsanitize=address and use -g only.
debug: $(SRCS)
	$(CC) $(CFLAGS) -fsanitize=address -o simulator_debug.exe $^ $(LDFLAGS)
	@echo Built: simulator_debug.exe (with AddressSanitizer)

# ── clean ───────────────────────────────────────────────────
clean:
	del /Q $(TARGET) 2>nul || true
	del /Q tests\run_tests.exe 2>nul || true
	del /Q simulator_debug.exe 2>nul || true
	del /Q tests\*.csv 2>nul || true
	@echo Cleaned.

.PHONY: all test debug clean
