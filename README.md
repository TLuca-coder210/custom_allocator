# Custom Memory Allocator (ARM64 / Apple Silicon)

A minimal, production-grade custom memory allocator written in C, inspired by standard `malloc`. Built specifically with awareness of Apple Silicon (ARM64) architecture constraints, utilizing system-level memory mapping and a doubly linked list structure.

---

## Features

- **Direct Kernel Allocation (`mmap`)**: Bypasses standard libc heap routines by requesting anonymous memory pages directly from the macOS XNU kernel using `mmap`.
- **Apple Silicon Page Awareness**: Configured for 16 KB virtual memory pages (`PAGE_SIZE = 16384`), rounding allocation requests to full page boundaries.
- **16-Byte Hardware Alignment**: Enforces strict 16-byte alignment on every payload address to comply with ARM64 NEON/SIMD vector instruction requirements and prevent alignment faults.
- **First-Fit Search Strategy**: Reuses previously allocated or freed blocks by traversing the metadata list.
- **Block Splitting**: Subdivides oversized pages into an allocated block and a remainder free block to reduce external and internal fragmentation.
