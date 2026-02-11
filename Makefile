# eBPF Android Bootstrap - Makefile
# ================================

# 配置变量
SDK_PATH := sdk/arm64
VMLINUX_PATH := $(SDK_PATH)/vmlinux/6.1/vmlinux.h
BUILD_DIR := build
SRC_DIR := src

# 工具链配置
CLANG ?= clang
LLC ?= llc
LLVM_STRIP ?= llvm-strip
BPFTOOL ?= bpftool

# Android NDK 配置 (用于编译用户态程序)
ANDROID_NDK_HOME ?= $(HOME)/Android/Sdk/ndk/27.0.12077973
NDK_TOOLCHAIN := $(ANDROID_NDK_HOME)/toolchains/llvm/prebuilt/linux-x86_64
CC_ANDROID := $(NDK_TOOLCHAIN)/bin/aarch64-linux-android30-clang

# eBPF 编译标志
BPF_CFLAGS := -target bpf \
              -D__TARGET_ARCH_arm64 \
              -O2 -g \
              -Wall -Werror \
              -I$(SDK_PATH)/include \
              -I$(SDK_PATH)/include/bpf \
              -I$(SDK_PATH)/vmlinux/6.1

# 用户态程序编译标志
USER_CFLAGS := -O2 -Wall \
               -I$(SDK_PATH)/include \
               -I$(BUILD_DIR) \
               -static

USER_LDFLAGS := -L$(SDK_PATH)/lib \
                -lbpf -lelf -lz \
                -static

# 自动发现源文件
BPF_SRCS := $(wildcard $(SRC_DIR)/*.bpf.c)
BPF_OBJS := $(patsubst $(SRC_DIR)/%.bpf.c,$(BUILD_DIR)/%.bpf.o,$(BPF_SRCS))
BPF_SKELS := $(patsubst $(SRC_DIR)/%.bpf.c,$(BUILD_DIR)/%.skel.h,$(BPF_SRCS))

USER_SRCS := $(filter-out $(BPF_SRCS),$(wildcard $(SRC_DIR)/*.c))
USER_BINS := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%_loader,$(USER_SRCS))

# 默认目标
.PHONY: all clean help

all: $(BUILD_DIR) $(BPF_OBJS) $(BPF_SKELS) $(USER_BINS)
	@echo "✅ Build complete!"
	@echo "📦 Artifacts in $(BUILD_DIR)/"
	@ls -la $(BUILD_DIR)/

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# 编译 eBPF 程序 (.bpf.c -> .bpf.o)
$(BUILD_DIR)/%.bpf.o: $(SRC_DIR)/%.bpf.c $(VMLINUX_PATH) | $(BUILD_DIR)
	@echo "🔨 Compiling eBPF: $<"
	$(CLANG) $(BPF_CFLAGS) -c $< -o $@
	@echo "📏 Size: $$(stat -c%s $@) bytes"

# 生成 skeleton 头文件 (.bpf.o -> .skel.h)
$(BUILD_DIR)/%.skel.h: $(BUILD_DIR)/%.bpf.o | $(BUILD_DIR)
	@echo "🦴 Generating skeleton: $@"
	$(BPFTOOL) gen skeleton $< > $@

# 编译用户态加载器 (.c -> _loader)
# 依赖对应的 skeleton 头文件
$(BUILD_DIR)/%_loader: $(SRC_DIR)/%.c $(BUILD_DIR)/%.skel.h | $(BUILD_DIR)
	@echo "🔨 Compiling loader: $<"
	$(CC_ANDROID) $(USER_CFLAGS) $< -o $@ $(USER_LDFLAGS)
	@echo "📏 Size: $$(stat -c%s $@) bytes"

# 仅编译 eBPF 程序（不需要 NDK）
.PHONY: bpf-only
bpf-only: $(BUILD_DIR) $(BPF_OBJS) $(BPF_SKELS)
	@echo "✅ eBPF programs and skeletons compiled!"

# 清理
clean:
	rm -rf $(BUILD_DIR)
	@echo "🧹 Cleaned!"

# 帮助信息
help:
	@echo "eBPF Android Bootstrap"
	@echo "======================"
	@echo ""
	@echo "Targets:"
	@echo "  all       - Build all eBPF programs, skeletons, and loaders"
	@echo "  bpf-only  - Build only eBPF programs and skeletons (no NDK required)"
	@echo "  clean     - Remove build artifacts"
	@echo "  help      - Show this help message"
	@echo ""
	@echo "Variables:"
	@echo "  ANDROID_NDK_HOME - Path to Android NDK (default: $(ANDROID_NDK_HOME))"
	@echo "  CLANG            - Clang compiler (default: $(CLANG))"
	@echo "  BPFTOOL          - BPF tool (default: $(BPFTOOL))"
	@echo ""
	@echo "Files found:"
	@echo "  BPF sources: $(BPF_SRCS)"
	@echo "  User sources: $(USER_SRCS)"