libbpf Android SDK
==================

Architecture: arm64 (arm64-v8a)
Android API Level: 30
NDK Version: r27b

Versions:
- libbpf: v1.5.0
- elfutils: 0.191
- zlib: 1.3.1

Directory Structure:
- include/  : Header files (bpf/, elf.h, libelf.h, gelf.h, etc.)
- lib/      : Static libraries (libbpf.a, libelf.a, libz.a)
- lib/pkgconfig/ : pkg-config files

Usage in Android.bp (AOSP):
--------------------------
cc_library_static {
    name: "libbpf",
    export_include_dirs: ["include"],
    srcs: ["lib/libbpf.a"],
    // ... 
}

Usage with ndk-build:
--------------------
LOCAL_STATIC_LIBRARIES := libbpf libelf libz

Usage with CMake:
----------------
target_include_directories(your_target PRIVATE ${SDK_PATH}/include)
target_link_libraries(your_target ${SDK_PATH}/lib/libbpf.a ${SDK_PATH}/lib/libelf.a ${SDK_PATH}/lib/libz.a)

Build Date: Wed Jan 28 03:58:37 UTC 2026
