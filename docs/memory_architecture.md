# Nano OS Memory Architecture

This document outlines the memory layout, physical memory management, paging, and kernel heap allocation mechanisms used in Nano OS.

## 1. System Memory Map

Nano OS utilizes a heavily structured memory map during the boot process and kernel execution. Below is the layout of physical memory in the early stages of the system:

| Address Range       | Size   | Usage Description                                                                 |
|---------------------|--------|-----------------------------------------------------------------------------------|
| `0x0000` - `0x4FFF` | ~20 KB | BIOS Data Area and unused low memory.                                             |
| `0x5000` - `0x51FF` | 512 B  | **VBE Mode Info Block**: Stored by the bootloader to pass GUI metrics to the C kernel. |
| `0x7C00` - `0x7DFF` | 512 B  | **Boot Sector**: Stage 1 bootloader execution.                                    |
| `0x9000` - `0x9FFF` | 4 KB   | **Page Directory**: Global page directory table.                                  |
| `0xA000` - `0xAFFF` | 4 KB   | **Page Table**: First page table (identity maps the first 4MB of memory).         |
| `0x10000`           | Variable| **Kernel Code/Data**: Shifted up from 0x1000 to 0x10000 to accommodate more lower memory features. |
| `0x30000`           | Variable| **Initrd (RAM Disk)**: Initial ramdisk loaded by BIOS interrupt 0x13 before protected mode. |
| `0x90000`           | ~64 KB | **Kernel Stack**: Initial downward-growing stack set by the bootloader for the C kernel. |
| `0x100000` (1MB)    | Variable| **PMM Free Pool**: Starting address of free frames available for allocation.      |
| `0x200000` (2MB)    | 512 KB | **Kernel Heap**: Bounded dynamic heap used for `kmalloc` and `kfree`.             |

## 2. Physical Memory Manager (PMM)

The PMM (`pmm.c`) tracks available physical memory using a **Bitmap-based allocator**.

- **Page Size**: 4096 bytes (4 KB).
- **Max Memory Limit**: Hardcoded to 64 MB (designed for typical lightweight virtual machine execution).
- **Bitmap Size**: 512 integers (32-bit), representing 16,384 distinct blocks.
- **Initialization**: `init_pmm(start_addr)` reserves all blocks initially, and then explicitly frees the region starting from `start_addr` up to the 64 MB limit.

**Core API:**
- `pmm_alloc_block()`: Finds the first free bit, marks it used, and returns the physical 4KB frame address.
- `pmm_free_block(addr)`: Clears the bit for the corresponding physical address.
- `pmm_reserve_region(addr, size)` / `pmm_free_region(addr, size)`: Helper functions used to map contiguous chunks (e.g., reserving memory for the heap and Initrd).

## 3. Paging & Virtual Memory

Virtual memory is enabled immediately in the assembly bootloader before jumping to the C kernel.

- **Identity Mapping**: The first 4MB of memory is identity-mapped (Virtual Address = Physical Address). This ensures that the bootloader, kernel, PMM, and memory-mapped IO function seamlessly.
- **Page Size Extension (PSE)**: Bit 4 of `CR4` is enabled. If a VESA Linear Framebuffer (LFB) is detected, it is mapped into virtual memory using **4 MB pages**. This eliminates the need to populate thousands of 4KB page tables just to draw to the screen.

## 4. Dynamic Heap (`kmalloc`)

Nano OS features a custom kernel heap (`kmalloc.c`) based on a first-fit block algorithm with header tracking.

- **Initialization**: `kmalloc_init(0x200000, 0x80000)` sets up a 512KB heap at the 2MB mark.
- **Block Tracking**: Each allocation has a preceding `kblock_header_t` containing:
  - `unsigned int size`
  - `unsigned int free`
  - `struct kblock_header* next`
- **Allocation**: `kmalloc(size)` scans the linked list for the first `free == 1` block large enough to fit the request. If the block has excess space, it is split into two blocks.
- **Deallocation**: `kfree(ptr)` marks the block's `free` flag as `1` and attempts to coalesce it with adjacent free blocks to prevent fragmentation.