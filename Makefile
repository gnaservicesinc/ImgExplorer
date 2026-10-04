PROGRAM := imgexplorer
BUILD_DIR := build

CC ?= cc
AR ?= ar
PYTHON ?= python3

OPT ?= -O2
DEBUG ?= -g
EXTRA_CFLAGS ?=
EXTRA_LDFLAGS ?=

C_STANDARD := -std=c11
DEPENDENCY_FLAGS := -MMD -MP
APP_WARNINGS := -Wall -Wextra -Wpedantic -Wconversion -Wshadow \
	-Wstrict-prototypes -Wmissing-prototypes -Werror
CPPFLAGS := -Isrc -Ithird_party/stb -Ithird_party/tinyexr/include \
	-Ithird_party/tinyexr/src -DEXR_NO_ZSTD
APP_CFLAGS := $(C_STANDARD) $(APP_WARNINGS) $(OPT) $(DEBUG) $(EXTRA_CFLAGS)
THIRD_PARTY_CFLAGS := $(C_STANDARD) $(OPT) $(DEBUG) $(EXTRA_CFLAGS) -w
LDLIBS := -lm

APP_SOURCES := src/main.c src/image.c src/exr_loader.c src/compare.c src/export.c \
	src/stb_image_impl.c
APP_OBJECTS := $(patsubst src/%.c,$(BUILD_DIR)/app/%.o,$(APP_SOURCES))
CORE_OBJECTS := $(filter-out $(BUILD_DIR)/app/main.o,$(APP_OBJECTS))

TINYEXR_EXCLUDED := \
	third_party/tinyexr/src/exr_freestanding.c \
	third_party/tinyexr/src/exr_gpu_cuda.c \
	third_party/tinyexr/src/exr_vk_vulkan.c \
	third_party/tinyexr/src/exr_zstd.c
TINYEXR_SOURCES := $(filter-out $(TINYEXR_EXCLUDED),\
	$(wildcard third_party/tinyexr/src/*.c))
TINYEXR_OBJECTS := $(patsubst third_party/tinyexr/src/%.c,\
	$(BUILD_DIR)/tinyexr/%.o,$(TINYEXR_SOURCES))
TINYEXR_LIBRARY := $(BUILD_DIR)/libtinyexr.a
DEPENDENCY_FILES := $(APP_OBJECTS:.o=.d) $(TINYEXR_OBJECTS:.o=.d)

TEST_CORE := $(BUILD_DIR)/tests/test_core
TEST_EXR_GENERATOR := $(BUILD_DIR)/tests/generate_exr_fixtures
TEST_FIXTURES := $(BUILD_DIR)/test-fixtures

.PHONY: all clean debug release test sanitize help

all: $(PROGRAM)

$(PROGRAM): $(APP_OBJECTS) $(TINYEXR_LIBRARY)
	$(CC) $(EXTRA_LDFLAGS) $(APP_OBJECTS) $(TINYEXR_LIBRARY) $(LDLIBS) -o $@

$(BUILD_DIR)/app/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(APP_CFLAGS) $(DEPENDENCY_FLAGS) -c $< -o $@

$(BUILD_DIR)/app/stb_image_impl.o: src/stb_image_impl.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(THIRD_PARTY_CFLAGS) $(DEPENDENCY_FLAGS) -c $< -o $@

$(BUILD_DIR)/tinyexr/%.o: third_party/tinyexr/src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(THIRD_PARTY_CFLAGS) $(DEPENDENCY_FLAGS) -c $< -o $@

$(TINYEXR_LIBRARY): $(TINYEXR_OBJECTS)
	@mkdir -p $(dir $@)
	$(AR) rcs $@ $^

debug:
	$(MAKE) clean
	$(MAKE) OPT=-O0 DEBUG=-g3 all

release:
	$(MAKE) clean
	$(MAKE) OPT=-O3 DEBUG= all

$(TEST_EXR_GENERATOR): tests/generate_exr_fixtures.c $(TINYEXR_LIBRARY)
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(APP_CFLAGS) $< $(TINYEXR_LIBRARY) $(LDLIBS) -o $@

$(TEST_CORE): tests/test_core.c $(CORE_OBJECTS) $(TINYEXR_LIBRARY)
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(APP_CFLAGS) $(EXTRA_LDFLAGS) $< $(CORE_OBJECTS) \
		$(TINYEXR_LIBRARY) $(LDLIBS) -o $@

test: $(PROGRAM) $(TEST_EXR_GENERATOR) $(TEST_CORE)
	@mkdir -p $(TEST_FIXTURES)
	$(PYTHON) tests/generate_png_fixtures.py $(TEST_FIXTURES)
	$(TEST_EXR_GENERATOR) $(TEST_FIXTURES)
	$(TEST_CORE) $(TEST_FIXTURES)
	sh tests/test_menu.sh ./$(PROGRAM) $(TEST_FIXTURES)

sanitize:
	$(MAKE) clean
	$(MAKE) OPT=-O1 DEBUG=-g3 \
		EXTRA_CFLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer" \
		EXTRA_LDFLAGS="-fsanitize=address,undefined" test

clean:
	rm -rf $(BUILD_DIR) $(PROGRAM)

help:
	@echo "make                 Build $(PROGRAM) with -O2 -g"
	@echo "make release         Clean build with -O3 and no debug symbols"
	@echo "make debug           Clean build with -O0 -g3"
	@echo "make test            Build and run loader/menu/export tests"
	@echo "make sanitize        Run tests with AddressSanitizer and UBSan"
	@echo "make clean && make OPT=-O3 DEBUG=-g  Use custom compile flags"
	@echo "make clean           Remove generated build files"

-include $(DEPENDENCY_FILES)
