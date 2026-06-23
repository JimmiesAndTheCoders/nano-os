# Nano OS Release Notes

## Version 1.0.0

### Date: 2026-06-22

We are thrilled to announce the completion of Phase 1 of Nano OS! This release represents the transition of Nano OS from a basic bootloader into a functional, multi-tasking hobby operating system.

#### Key Features & Achievements

- Bootloader & Kernel: Initialized a robust boot system with support for kernel loading.
- Memory Management: Implemented a bitmap-based memory allocator, virtual memory support, and dynamic kmalloc/kfree capabilities.
- Multitasking: Added a Programmable Interval Timer (PIT) and preemptive context switching for true multitasking.
- System Services: Created a software interrupt interface for user-space programs to communicate with the kernel.
- Security: Successfully transitioned execution from ring 0 to ring 3 to enhance system isolation.
- Graphics: Enabled VESA/VBE modes, providing the foundation for high-resolution graphics.
- Reliability: Integrated a C utility test harness and streamlined the build system for seamless operation across Linux, macOS, and Windows.

#### Build & Deployment

Nano OS is now built using an automated pipeline via GitHub Actions.

## Version 1.1.0

Added the mouse track on status.

## Version 1.2.0

Added commands:

- `echo [text]`
- `pwd`
- `cd [dir]`
- `grep [pat] [f]`
- `touch [file]`
- `mkdir [dir]`
- `cnode [file]`

## Version 1.3.0

Added feature:
- A PCI bus enumerator to detect, parse configuration spaces, and configure hardware

Added command:
- `pci`

Every tagged release (v*) will manually produce a nano_os-[VERSION].img file, available in the "Releases" section of the repository.