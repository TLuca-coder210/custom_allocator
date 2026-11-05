# Custom Memory Allocator (ARM64 / Apple Silicon)

A minimal, high-performance custom memory allocator written in C, inspired by standard `malloc` and modern enterprise allocators like `tcmalloc`. Built specifically with awareness of Apple Silicon (ARM64) architecture constraints, it utilises system-level memory mapping, a First-Fit doubly linked list for large allocations, and a **Thread-Local Slab Cache** for lock-free small allocations.

---

## Features

**Core Engine (Large Allocations & Fundamentals)**
- **Direct Kernel Allocation (`mmap`)**: Bypasses standard libc heap routines by requesting anonymous memory pages directly from the macOS XNU kernel using `mmap`.
- **Apple Silicon Page Awareness**: Configured for 16 KB virtual memory pages (`PAGE_SIZE = 16384`), rounding initial system requests to page boundaries.
- **16-Byte Hardware Alignment**: Enforces strict 16-byte alignment on every payload address to comply with ARM64 NEON/SIMD vector instruction requirements.
- **First-Fit Search Strategy**: Reuses previously allocated or freed blocks by traversing the metadata list.
- **Block Splitting**: Subdivides oversized blocks into an active allocation and a remainder free block to reduce fragmentation.
- **O(1) Bidirectional Coalescing (`my_free`)**: Merges newly freed memory with adjacent free blocks (both left and right) in constant time to reclaim contiguous memory space.

**Thread-Local Cache (Small Allocations & Optimization)**
- **Lock-Free Thread Cache**: Small allocations (≤ 256 bytes) are served from a thread-local (`__thread`) stack of size-segregated bins, entirely bypassing locks and global lists.
- **Zero-Overhead Slab Allocation**: Eliminates per-block metadata (no 16-byte header per slice). A single metadata header is placed at the start of a 16KB page, achieving ~0.09% overhead instead of 50%.
- **O(1) Headerless Freeing**: Uses bitwise masking (`ptr & ~(PAGE_SIZE - 1)`) to locate the slab header in constant time during `tc_free`, instantly routing the block back to its thread-local bin.
- **Dual-Allocator Bypass via Magic Number**: Intelligently routes memory during `free` and `realloc`. A `0xCAFEBABE` magic signature in the slab header allows the allocator to instantly distinguish between small cached blocks and large First-Fit blocks.
- **Smart Reallocation**: `tc_realloc` avoids unnecessary `memcpy` operations by checking if the resized payload still fits within the same size-class bin.

---

## Architecture & Logic

The allocator operates on a **Two-Tier System** to balance memory efficiency with maximum CPU performance:

### 1. The Fast Path (Thread Cache)
When a user requests a small block (e.g., 32 bytes), the allocator rounds it to the nearest 16-byte boundary and checks a thread-local array of bins. 
- **Cache Hit (O(1)):** If the bin has available blocks, it pops the top block and returns it instantly.
- **Cache Miss (Slab Refill):** If the bin is empty, `tc_refill` directly requests a strictly aligned 16KB page from the OS via `mmap`. It places a single `slab_header_t` at the very beginning of the page, slicing the remainder into perfect 32-byte chunks, linking them into the bin, and returning one to the user.

### 2. The Math Behind O(1) Deallocation
Standard allocators place a header right before the payload, causing severe internal fragmentation for small sizes. This allocator uses BiBOP (Bag in Box of Pages) logic. 
When `tc_free(ptr)` is called, the allocator does not know the size of the block. Because `mmap` guarantees the 16KB page starts at a multiple of `16384`, the allocator simply performs a bitwise AND operation on the pointer to strip the lower bits. This instantly resolves to the page's start address, where the `slab_header_t` resides. The size is read, and the block is pushed back onto the corresponding cache bin in O(1) time.

### 3. The Slow Path (First-Fit Engine)
Allocations larger than 256 bytes bypass the thread cache entirely. They are routed to the underlying `my_alloc` First-Fit engine, which dynamically manages variable-sized blocks with standard inline headers, handling block splitting and neighbor coalescing. If a large block is freed via the Thread Cache interface, the system detects the absence of the magic number (`0xCAFEBABE`) and safely routes it back to `my_free`.

---

## Testing & Verification

The allocator includes a test suite (`my_test.c`) verifying core runtime guarantees via `assert`:

- **Pointer Alignment**: Asserts `(uintptr_t)ptr % 16 == 0
