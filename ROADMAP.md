# Nano OS Development Roadmap

This roadmap outlines the planned features and improvements for Nano OS. The project is in active development, and we welcome contributions from the community. Below are the key milestones and features we aim to implement in the coming months.

<details>
<summary><b>[COMPLETED] Phase 1 (MS-DOS like hobby OS)</b></summary>

- [X] Initialize the bootloader and setup the kernel.
- [X] Implement a bitmap-based memory allocator to track free/used frames.
- [X] Enable virtual memory to support isolation between kernel and user space.
- [X] Implement `kmalloc` and `kfree` to support dynamic memory allocation in the kernel.
- [X] Implement the Programmable Interval Timer (PIT) to enable system ticks and multitasking.
- [X] Add preemptive multitasking (context switching) to run multiple "tasks" at once.
- [X] Implement a simple RAM-based file system (like Initrd) to store and load files.
- [X] Create an interface (software interrupts) for user programs to request kernel services.
- [X] Transition from ring 0 to ring 3 execution to secure the system.
- [X] Enable VESA/VBE modes for high-resolution graphics and basic drawing primitives.
- [X] Add a test harness for C utility functions to ensure stability.
- [X] Expand technical docs for driver API and memory architecture.
- [X] Clean up build system to support Linux, macOS, and Windows host build environments equally.

</details>

<br>

Phase 2 (Fully GUI operating system): 
- [ ] Implement a PS/2 mouse driver with interrupt-driven (IRQ 12) coordinate tracking.
- [ ] Create a basic Window Manager (compositing, overlapping windows, dragging, and resizing).
- [ ] Develop a standard GUI Toolkit (buttons, text boxes, dialogs, and sliders).
- [ ] Implement an ATA/IDE PIO disk driver for persistent read/write storage.
- [ ] Introduce a full file system (e.g., FAT32 or ext2) to replace the read-only initrd.
- [ ] Add an ELF binary parser to dynamically load and execute user-space applications from disk.
- [ ] Implement Inter-Process Communication (IPC) (e.g., message passing, shared memory, pipes).
- [ ] Add a PCI bus enumerator to detect and configure connected hardware devices dynamically.
- [ ] Implement a Real-Time Clock (RTC) driver for accurate system time and date tracking.
- [ ] Develop a GUI terminal emulator to upgrade the existing Nano Shell with history and scripting.
- [ ] Add basic networking support (e.g., RTL8139 Ethernet driver, custom TCP/IP stack) for connectivity.
- [ ] Add support for sound hardware (e.g., AC97, Intel HDA, or SoundBlaster 16).
- [ ] Add support for additional hardware devices (e.g., USB controllers, SATA drives).
- [ ] Optimize the kernel and VESA/VBE drawing algorithms for hardware performance and stability.

This roadmap is subject to change as the project evolves, but it provides a clear direction for the development of Nano OS. We encourage contributors to pick any of the open tasks and help us bring Nano OS to life!
