// SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
// Raw Tracepoint example for Android (Pixel 6 / Android 16)
// 跟踪文件打开操作，输出进程名和文件名

#include "vmlinux.h"
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>
#include <bpf/bpf_core_read.h>

// ARM64 系统调用号
#define __NR_openat 56

// 定义事件结构
struct event {
    u32 pid;
    u32 tid;
    u32 uid;
    char comm[16];
    char filename[256];
};

// Ring buffer map
struct {
    __uint(type, BPF_MAP_TYPE_RINGBUF);
    __uint(max_entries, 256 * 1024);
} events SEC(".maps");

// Raw tracepoint 参数结构 (sys_enter)
// 注意：raw tracepoint 的参数是通过 pt_regs 传递的
SEC("raw_tracepoint/sys_enter")
int raw_tp_sys_enter(struct bpf_raw_tracepoint_args *ctx)
{
    // ctx->args[0] = pt_regs (寄存器)
    // ctx->args[1] = syscall number
    unsigned long syscall_nr = ctx->args[1];
    
    // 只处理 openat 系统调用
    if (syscall_nr != __NR_openat)
        return 0;

    struct event *e;
    u64 pid_tgid;
    struct pt_regs *regs = (struct pt_regs *)ctx->args[0];

    // 分配 ring buffer 空间
    e = bpf_ringbuf_reserve(&events, sizeof(*e), 0);
    if (!e)
        return 0;

    // 获取进程信息
    pid_tgid = bpf_get_current_pid_tgid();
    e->pid = pid_tgid >> 32;
    e->tid = (u32)pid_tgid;
    e->uid = bpf_get_current_uid_gid() & 0xFFFFFFFF;

    // 获取进程名 (comm)
    bpf_get_current_comm(&e->comm, sizeof(e->comm));

    // openat(int dirfd, const char *pathname, int flags, mode_t mode)
    // ARM64: x0=dirfd, x1=pathname, x2=flags, x3=mode
    // 文件名是第二个参数 (x1 寄存器)
    unsigned long filename_ptr;
    bpf_probe_read_kernel(&filename_ptr, sizeof(filename_ptr), &regs->regs[1]);
    bpf_probe_read_user_str(&e->filename, sizeof(e->filename), (const char *)filename_ptr);

    // 提交事件
    bpf_ringbuf_submit(e, 0);

    return 0;
}

char LICENSE[] SEC("license") = "GPL";