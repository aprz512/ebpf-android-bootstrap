# eBPF Android Bootstrap 🚀

一个用于在 Android 设备上开发 eBPF 程序的脚手架项目。开发者只需要编写代码，通过 GitHub Actions 自动编译，即可下载可在 Android 上运行的 eBPF 程序。

## ✨ 特性

- 📦 预编译的 ARM64 SDK（libbpf v1.5.0 + elfutils + zlib）
- 🔧 完整的 vmlinux.h（内核 6.1）
- 🤖 GitHub Actions 自动构建
- 📱 支持 Android API Level 30+
- 🛠 简单的 Makefile 构建系统

## 📁 项目结构

```
.
├── .github/workflows/    # GitHub Actions 工作流
│   └── build.yml
├── sdk/                  # 预编译的 SDK
│   └── arm64/
│       ├── include/      # 头文件
│       ├── lib/          # 静态库
│       └── vmlinux/      # 内核类型定义
├── src/                  # 你的代码放这里！
│   ├── example.bpf.c     # eBPF 程序示例
│   └── example.c         # 用户态加载器示例
├── build/                # 编译输出目录
├── Makefile              # 构建脚本
└── README.md
```

## 🚀 快速开始

### 方式一：使用 GitHub Actions（推荐）

1. **Fork 这个仓库**

2. **编写你的 eBPF 程序**
   - 在 `src/` 目录下创建 `yourprogram.bpf.c`（eBPF 内核态程序）
   - 在 `src/` 目录下创建 `yourprogram.c`（用户态加载器）

3. **提交代码**
   ```bash
   git add src/
   git commit -m "Add my eBPF program"
   git push
   ```

4. **下载编译产物**
   - 进入 GitHub 仓库的 "Actions" 页面
   - 找到最新的构建任务
   - 下载 `ebpf-android-binaries` artifact

### 方式二：本地构建

#### 前置要求

- Clang/LLVM 15+
- bpftool（用于生成 skeleton）
- Android NDK r27b+（仅编译用户态加载器需要）

```bash
# Ubuntu/Debian 安装 bpftool
sudo apt-get install linux-tools-common linux-tools-generic
```

#### 仅编译 eBPF 程序

```bash
# 不需要 NDK，只需要 clang
make bpf-only
```

#### 编译完整程序

```bash
# 设置 NDK 路径
export ANDROID_NDK_HOME=/path/to/android-ndk-r27b

# 编译
make all
```

## 📝 编写你自己的 eBPF 程序

### 1. 创建 eBPF 程序（内核态）

创建 `src/myprogram.bpf.c`：

```c
#include "vmlinux.h"
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>

SEC("tracepoint/syscalls/sys_enter_write")
int trace_write(struct trace_event_raw_sys_enter *ctx)
{
    u32 pid = bpf_get_current_pid_tgid() >> 32;
    bpf_printk("PID %d called write()\n", pid);
    return 0;
}

char LICENSE[] SEC("license") = "GPL";
```

### 2. 创建加载器（用户态）

创建 `src/myprogram.c`：

```c
#include <stdio.h>
#include <bpf/libbpf.h>
#include <bpf/bpf.h>

int main(int argc, char **argv)
{
    struct bpf_object *obj;
    struct bpf_program *prog;
    struct bpf_link *link;
    
    obj = bpf_object__open_file("myprogram.bpf.o", NULL);
    bpf_object__load(obj);
    
    prog = bpf_object__find_program_by_name(obj, "trace_write");
    link = bpf_program__attach(prog);
    
    printf("Running... Press Ctrl+C to exit\n");
    while(1) { sleep(1); }
    
    bpf_link__destroy(link);
    bpf_object__close(obj);
    return 0;
}
```

## 📱 在 Android 上运行

```bash
# 推送文件到设备
adb push build/myprogram.bpf.o /data/local/tmp/
adb push build/myprogram_loader /data/local/tmp/

# 运行（需要 root）
adb shell
su
cd /data/local/tmp
chmod +x myprogram_loader
./myprogram_loader
```

## 🔍 调试

### 查看 eBPF 日志

```bash
adb shell "cat /sys/kernel/debug/tracing/trace_pipe"
```

### 查看加载的 eBPF 程序

```bash
adb shell "bpftool prog list"
```

## ⚠️ Android 设备要求

- 内核版本 4.9+（推荐 5.4+）
- 启用 eBPF 支持的内核配置：
  - `CONFIG_BPF=y`
  - `CONFIG_BPF_SYSCALL=y`
  - `CONFIG_BPF_JIT=y`
- Root 权限

## 📚 常用 eBPF 程序类型

| 类型 | 说明 | 示例 |
|------|------|------|
| `tracepoint/*` | 跟踪内核静态跟踪点 | 系统调用监控 |
| `kprobe/*` | 动态内核函数跟踪 | 函数调用分析 |
| `uprobe/*` | 用户态函数跟踪 | 应用程序分析 |
| `socket_filter` | 网络数据包过滤 | 流量分析 |
| `xdp` | 高性能数据包处理 | 防火墙 |

## 🤝 贡献

欢迎提交 Issue 和 Pull Request！

## 📄 License

本项目采用 [MIT License](LICENSE)。

eBPF 程序示例采用 `GPL-2.0 OR BSD-3-Clause` 双重许可。
