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

Phase 2: Core Subsystems & Persistent Storage

- [X] Add a PCI bus enumerator to detect, parse configuration spaces, and configure hardware.
- [X] Add support for MSI/MSI-X (Message Signaled Interrupts) in the PCI driver.
- [X] Implement a Real-Time Clock (RTC) / CMOS driver for accurate system time tracking.
- [ ] Upgrade timer support to the High Precision Event Timer (HPET) or APIC timer.
- [ ] Implement an ATA/IDE PIO disk driver for basic read/write storage.
- [ ] Upgrade ATA driver to support DMA (Direct Memory Access) for faster disk I/O.
- [ ] Create a Virtual File System (VFS) abstraction (open, read, write, close, ioctl, mount points).
- [ ] Implement a FAT32 driver (MBR parsing, Boot sector, FAT tables, clusters, and Long File Names).
- [ ] Implement an Ext2 file system driver (Superblock, block groups, inodes, directory parsing).
- [ ] Build a page cache and buffer cache mechanism to optimize disk read/write operations.
- [ ] Add an ELF binary parser to dynamically load, map, and execute user-space applications.
- [ ] Implement dynamic linking support for shared libraries (.so files).
- [ ] Pass environment variables and command-line arguments (argc/argv) to user programs.
- [ ] Implement Unix-like Signals for basic process control (e.g., SIGKILL, SIGINT).
- [ ] Implement Inter-Process Communication (IPC) via pipes, mailboxes, and shared memory (mmap).
- [ ] Port a standard C library (libc) tailored to Nano OS system calls for user applications.

Phase 3: Desktop Environment & Windowing

- [X] Implement a PS/2 mouse driver with interrupt-driven (IRQ 12) coordinate tracking.
- [ ] Implement comprehensive keyboard mapping (scancodes to ASCII/Unicode, modifier keys).
- [ ] Build an input event queue system to route keystrokes and mouse events to the focused process.
- [ ] Implement double-buffering and hardware cursor rendering to eliminate screen tearing.
- [ ] Create a Window Server compositor architecture (z-ordering, overlapping, alpha blending).
- [ ] Implement clipping regions and "dirty rectangles" algorithms to optimize UI redraws.
- [ ] Add support for dragging, resizing, maximizing, and closing windows.
- [ ] Develop a standard GUI Toolkit (base Widget class, event callbacks, rendering primitives).
- [ ] Implement core UI controls: buttons, text boxes, dialogs, check boxes, scrollbars, and sliders.
- [ ] Integrate a font rendering engine (e.g., FreeType port for TrueType/OpenType or custom bitmap fonts).
- [ ] Implement anti-aliasing for text and geometric primitives.
- [ ] Develop a GUI terminal emulator to upgrade Nano Shell (ANSI escape codes, scrollback, PTY support).
- [ ] Create a basic Desktop environment (Taskbar/dock, desktop background, clock widget).
- [ ] Build a graphical File Explorer with icon views and basic file operations (copy, paste, delete).

Phase 4: Networking & Advanced Peripherals

- [ ] Implement a loopback network interface (127.0.0.1) and local routing table.
- [ ] Add basic Ethernet support via RTL8139 or Intel PRO/1000 (e1000) driver using DMA rings.
- [ ] Build a custom TCP/IP Network Stack - Layer 2: Ethernet framing, ARP caching.
- [ ] Build a custom TCP/IP Network Stack - Layer 3: IPv4 packet routing, ICMP (Ping).
- [ ] Build a custom TCP/IP Network Stack - Layer 4: UDP datagrams, TCP state machine (sliding windows, handshakes).
- [ ] Implement standard BSD-style socket APIs (socket, bind, listen, accept, connect).
- [ ] Establish a USB Core Subsystem (device enumeration, configuration descriptors, endpoints).
- [ ] Write USB host controller drivers (UHCI/OHCI for USB 1.1, EHCI for USB 2.0).
- [ ] Implement USB Human Interface Device (HID) support for modern USB keyboards and mice.
- [ ] Add USB Mass Storage class support for reading from USB flash drives.
- [ ] Add SATA Advanced Host Controller Interface (AHCI) support for modern high-speed disks.
- [ ] Add support for sound hardware (e.g., Intel HDA or AC97).
- [ ] Implement PCM audio stream buffers and DMA audio playback.
- [ ] Create a user-space audio daemon/mixer to combine multiple audio streams.

Phase 5: Optimization, Security & System Polish

- [ ] Implement Symmetric Multiprocessing (SMP) via ACPI MADT parsing and waking Application Processors (APs).
- [ ] Add Spinlocks, Mutexes, and Semaphores for kernel and user-space concurrency control.
- [ ] Implement a modern thread scheduler (e.g., CFS-like) and Thread Local Storage (TLS).
- [ ] Develop a Slab allocator for efficient caching of frequently used kernel objects.
- [ ] Implement demand paging / lazy loading to reduce initial program memory footprints.
- [ ] Enforce strict user permissions (UID/GID) and file access control lists (ACLs).
- [ ] Implement SMAP/SMEP (Supervisor Mode Access/Execution Prevention) to protect the kernel from user-space.
- [ ] Implement the NX (No-Execute) bit for the stack to prevent buffer overflow execution.
- [ ] (Stretch Goal) Introduce Address Space Layout Randomization (ASLR).
- [ ] Create a /proc or /sys virtual filesystem for exposing kernel metrics to user-space.
- [ ] Develop a graphical System Monitor (Task Manager) to track processes, CPU cores, and memory allocation.
- [ ] Provide comprehensive crash reporting (Kernel Panic/BSOD) with stack unwinding and register dumps.
- [ ] Add a graphical boot splash screen and a user login/authentication screen.

This roadmap is subject to change as the project evolves, but it provides a clear direction for the development of Nano OS. We encourage contributors to pick any of the open tasks and help us bring Nano OS to life!
