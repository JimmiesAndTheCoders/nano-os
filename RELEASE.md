# Nano OS Release Notes

## Version 1

### Version 1.0.0

**Date: 2026-06-22**

We are thrilled to announce the completion of Phase 1 of Nano OS! This release represents the transition of Nano OS from a basic bootloader into a functional, multi-tasking hobby operating system.

#### Key Features & Achievements

- Bootloader & Kernel: Initialized a robust boot system with support for kernel loading.
- Memory Management: Implemented a bitmap-based memory allocator, virtual memory support, and dynamic kmalloc/kfree capabilities.
- Multitasking: Added a Programmable Interval Timer (PIT) and preemptive context switching for true multitasking.
- System Services: Created a software interrupt interface for user-space programs to communicate with the kernel.
- Security: Successfully transitioned execution from ring 0 to ring 3 to enhance system isolation.
- Graphics: Enabled VESA/VBE modes, providing the foundation for high-resolution graphics.
- Reliability: Integrated a C utility test harness and streamlined the build system for seamless operation across Linux, macOS, and Windows.

### Version 1.1.0

**Date: 2026-06-23**

Added the mouse track on status.

### Version 1.2.0

**Date: 2026-06-23**

Added commands:

- `echo [text]`
- `pwd`
- `cd [dir]`
- `grep [pat] [f]`
- `touch [file]`
- `mkdir [dir]`
- `cnode [file]`

### Version 1.3.0

**Date: 2026-06-23**

Added feature:
- A PCI bus enumerator to detect, parse configuration spaces, and configure hardware

Added command:
- `pci`

### Version 1.4.0

**Date: 2026-06-23**

Added feature:
- Support for MSI and MSI-X (Message Signaled Interrupts) in the PCI driver.
- Mapping capabilities to resolve BAR spaces via 4MB page structures (PSE).
- Setup of Local APIC MSR registers to trap signaled vectors.
- Interactive terminal subcommands enabling control of MSI modes.

### Version 1.5.0

**Date: 2026-06-23**

Added feature:
- A Real-Time Clock (RTC) / CMOS driver for accurate system time tracking.

### Version 1.6.0

**Date: 2026-06-23**

Added feature:
- Upgraded legacy system timer to a dynamically calibrated Local APIC (LAPIC) Timer.
- Masked legacy PIC PIT line to reduce bus interrupts.
- Implemented robust fallback logic to the PIT in cases where LAPIC initialization or calibration fails.

### Version 1.7.0

**Date: 2026-06-23**

Added feature:
- Developed an ATA/IDE PIO disk driver to support raw read, write, and identify diagnostics on storage drives.
- Upgraded the storage layer to support Bus Master IDE DMA with physical descriptor tables and page-aligned BSS bounce buffer structures.
- Exposed interactive diagnostics in the shell terminal to evaluate disk stability under both PIO and DMA channels.

### Version 1.8.0

**Date: 2026-06-23**

Added feature:
- A FAT32 driver (MBR parsing, Boot sector, FAT tables, clusters, and Long File Names).
- An Ext2 file system driver (Superblock, block groups, inodes, directory parsing).

### Version 1.9.0

**Date: 2026-06-24**

Added feature:
- A page cache and buffer cache mechanism to optimize disk read/write operations.

### Version 1.10.0

**Date: 2026-06-24**

Added feature:
- An ELF binary parser to dynamically load, map, and execute user-space applications.

### Version 1.11.0

**Date: 2026-06-24**

Added feature:
- A dynamic linking support for shared libraries (.so files), and pass environment variables and command-line arguments (argc/argv) to user programs.

### Version 1.12.0

**Date: 2026-06-24**

Added feature:
- Unix-like Signals for basic process control (e.g., SIGKILL, SIGINT).

### Version 1.13.0

**Date: 2026-06-25**

Added feature:
- Inter-Process Communication (IPC) subsystem.
- Dynamic pipes backed by ring buffers, message queue-driven mailboxes, and page-aligned shared memory segments.
- Mountable virtual filesystem mapping these structures directly to `/ipc/` for shell operations.
- Dynamic system calls for programmatic context exchanges between processes.
- Context execution test command `ipc-demo` to inspect concurrent IPC transfers.

## Version 2

### Version 2.0.0

#### Alpha 1

**Date: 2026-06-25**

Ported languages:
- Standard C Library

#### Alpha 2

**Date: 2026-06-25**

Added feature:
- CRT0
- CNode editor
- Text cursor

Updated feature:
- Keyboard now supports all letters, numbers, and symbols.

#### Alpha 3

**Date: 2026-06-25**

Added feature:
- Implement sbrk syscall and dynamic user heap.

#### Alpha 4

**Date: 2026-06-27**

Added feature:
- NCC host compiler tool and new user apps (user_prime, user_ipc_demo), and more.

#### Alpha 5

**Date: 2026-06-27**

Added feature:
- C library utilities.

Rewritten files:
- make_initrd.c

#### Alpha 6

**Date: 2026-06-27**

Added feature:
- calloc/realloc.

#### Alpha 7

**Date: 2026-06-27**

Added feature:
- Introduce FILE struct and stdin/stdout/stderr, define EOF, and expand stdio.h with prototypes for formatted and unformatted I/O.

Replaced:
- oOld syscall-based putchar/puts with implementations using read/write (unistd), implement vsprintf/vfprintf, printf/fprintf/sprintf, fputc/fputs, getchar/fgetc, and helper itoa functions.

Other:
- Supports %c, %s, %d/%i, %u, %x/%X and %%. Uses a 2048-byte stack buffer for vfprintf and preserves previous behavior by mapping streams to file descriptors 0/1/2.

#### Beta 1

**Date: 2026-06-27**

Added feature:
- Alternative compiler integration for core modules using the Zig build toolchain.
- Implemented core physical frame boundary checks in Zig (`src/zig_module.zig`) targeting `x86-freestanding-none`.
- Expanded the main Makefile build sequence to automatically compile and link Zig objects alongside the C/C++ kernel.

#### Beta 2

**Date: 2026-06-27**

Added feature:
- Rust kernel

#### Beta 3

**Date: 2026-06-28**

Added feature:
- Automated Rust FFI generation and build integration

#### Beta 4

**Date: 2026-06-28**

Rewritten files:
- Physical frame validation logic in Rust to guarantee spatial safety during allocation.

#### Beta 5

**Date: 2026-06-28**

Added features:
- zig_module.zig implementing zig_vfs_resolve_path, panic, and zig_verify_safety.
- MAX_MOUNTS macro to include/vfs.h.

Renamed stuff
- vfs_resolve_path in src/vfs.c now calls zig_vfs_resolve_path.

Moved stuff:
- mount table/mount_count moved to include/vfs.h and defined in src/vfs.c.

Provided stuff:
- memcpy and __zig_probe_stack in src/util.c to satisfy Zig/LLVM/GCC needs.

Updated files:
- Makefile ZIGFLAGS to include include/libc paths and disable Zig stack checks.

<br>

Nano OS is now built using an automated pipeline via GitHub Actions.
Every tagged release (v*) will automatically produce a nano_os-[VERSION].img file, available in the "Releases" section of the repository.
