// SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
// eBPF File Open Tracer for Android
// 使用 libbpf skeleton + raw_tracepoint（兼容 Android 16）

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <sys/resource.h>
#include <time.h>

#include <bpf/libbpf.h>
#include <bpf/bpf.h>

// 包含自动生成的 skeleton 头文件
#include "example.skel.h"

// 事件结构（与 eBPF 程序中定义的一致）
struct event {
    __u32 pid;
    __u32 tid;
    __u32 uid;
    char comm[16];
    char filename[256];
};

static volatile sig_atomic_t exiting = 0;

static void sig_handler(int sig)
{
    exiting = 1;
}

// libbpf 日志回调
static int libbpf_print_fn(enum libbpf_print_level level, const char *format, va_list args)
{
    // 生产环境可以只显示警告和错误
    if (level == LIBBPF_DEBUG)
        return 0;
    return vfprintf(stderr, format, args);
}

// Ring buffer 回调函数
static int handle_event(void *ctx, void *data, size_t data_sz)
{
    struct event *e = data;
    char time_str[16];
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    
    strftime(time_str, sizeof(time_str), "%H:%M:%S", tm);
    
    printf("[%s] %-16s PID:%-6d TID:%-6d UID:%-5d %s\n",
           time_str, e->comm, e->pid, e->tid, e->uid, e->filename);
    
    return 0;
}

int main(int argc, char **argv)
{
    struct example_bpf *skel = NULL;
    struct ring_buffer *rb = NULL;
    int err;
    
    // 设置 libbpf 日志
    libbpf_set_print(libbpf_print_fn);
    
    // 设置信号处理
    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);
    
    // 设置 rlimit（允许锁定内存）
    // eBPF 程序需要锁定内存用于 maps、程序代码等，默认限制可能不够(通常64KB)
    // 在 Android root 设备上通常已经是 unlimited，但为了兼容性仍然设置
    // 注意：Linux 5.11+ 内核使用 memcg 替代 RLIMIT_MEMLOCK，此设置可能无效
    struct rlimit rlim = {
        .rlim_cur = RLIM_INFINITY,
        .rlim_max = RLIM_INFINITY,
    };
    if (setrlimit(RLIMIT_MEMLOCK, &rlim)) {
        fprintf(stderr, "Warning: Failed to set RLIMIT_MEMLOCK: %s\n", 
                strerror(errno));
    }
    
    printf("===========================================\n");
    printf("  eBPF File Open Tracer for Android\n");
    printf("  (raw_tracepoint/sys_enter)\n");
    printf("===========================================\n\n");
    
    // 使用 skeleton 打开 eBPF 程序
    printf("Opening eBPF skeleton...\n");
    skel = example_bpf__open();
    if (!skel) {
        fprintf(stderr, "Failed to open BPF skeleton\n");
        return 1;
    }
    
    // 加载 eBPF 程序到内核
    printf("Loading eBPF program into kernel...\n");
    err = example_bpf__load(skel);
    if (err) {
        fprintf(stderr, "Failed to load BPF skeleton: %d\n", err);
        goto cleanup;
    }
    
    // Attach eBPF 程序（auto-attach）
    printf("Attaching eBPF program...\n");
    err = example_bpf__attach(skel);
    if (err) {
        fprintf(stderr, "Failed to attach BPF skeleton: %d\n", err);
        goto cleanup;
    }
    
    printf("eBPF program loaded and attached successfully!\n");
    
    // 使用 skeleton 直接获取 map fd（类型安全）
    int map_fd = bpf_map__fd(skel->maps.events);
    if (map_fd < 0) {
        fprintf(stderr, "Failed to get events map fd\n");
        err = -1;
        goto cleanup;
    }
    
    // 创建 ring buffer
    rb = ring_buffer__new(map_fd, handle_event, NULL, NULL);
    if (!rb) {
        fprintf(stderr, "Failed to create ring buffer\n");
        err = -1;
        goto cleanup;
    }
    
    printf("\n");
    printf("Tracing openat() syscalls... Press Ctrl+C to exit.\n");
    printf("-------------------------------------------\n");
    printf("%-10s %-16s %-10s %-10s %-8s %s\n", 
           "TIME", "COMM", "PID", "TID", "UID", "FILENAME");
    printf("-------------------------------------------\n");
    
    // 事件循环
    while (!exiting) {
        err = ring_buffer__poll(rb, 100 /* timeout ms */);
        if (err == -EINTR) {
            err = 0;
            break;
        }
        if (err < 0) {
            fprintf(stderr, "Error polling ring buffer: %d\n", err);
            break;
        }
    }
    
    printf("\n\nExiting...\n");
    err = 0;

cleanup:
    ring_buffer__free(rb);
    // skeleton 的 destroy 会自动清理所有 attach 的 links
    example_bpf__destroy(skel);
    
    return err != 0;
}