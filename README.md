# Custom Memory Allocator (ARM64 / Apple Silicon)

A minimal custom memory allocator written in C, inspired by standard `malloc`. Built specifically with awareness of Apple Silicon (ARM64) architecture constraints, utilizing system-level memory mapping and a doubly linked list structure.

---

## Features

- **Direct Kernel Allocation (`mmap`)**: Bypasses standard libc heap routines by requesting anonymous memory pages directly from the macOS XNU kernel using `mmap`.
- **Apple Silicon Page Awareness**: Configured for 16 KB virtual memory pages (`PAGE_SIZE = 16384`), rounding initial system requests to page boundaries.
- **16-Byte Hardware Alignment**: Enforces strict 16-byte alignment on every payload address to comply with ARM64 NEON/SIMD vector instruction requirements.
- **First-Fit Search Strategy**: Reuses previously allocated or freed blocks by traversing the metadata list.
- **Block Splitting**: Subdivides oversized blocks into an active allocation and a remainder free block to reduce fragmentation.
- **O(1) Bidirectional Coalescing (`my_free`)**: Merges newly freed memory with adjacent free blocks (both left and right) in constant time to reclaim contiguous memory space.

---

## Testing & Verification

The allocator includes a test suite (`my_test.c`) verifying core runtime guarantees via `assert`:

- **Pointer Alignment**: Asserts `(uintptr_t)ptr % 16 == 0` across variable-sized allocations (e.g., odd and non-power-of-two sizes like 5, 17, 33 bytes).
- **Data Integrity**: Validates physical Read/Write persistence across block boundaries without neighbor metadata corruption.
- **Memory Recycling**: Confirms immediate block reuse upon deallocation without invoking additional system calls.
