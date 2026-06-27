# Nano OS Development Roadmap

This roadmap outlines the planned features and improvements for Nano OS. Below are the milestones and features intended for implementation.

<details>
<summary><b>[COMPLETED] Phase 1: Core Boot & Memory Foundations</b></summary>

- [X] Initialize the bootloader and setup the kernel.
- [X] Implement a bitmap-based memory allocator to track free/used frames.
- [X] Enable virtual memory to support isolation between kernel and user space.
- [X] Implement `kmalloc` and `kfree` to support dynamic memory allocation in the kernel.
- [X] Implement the Programmable Interval Timer to enable system ticks and multitasking.
- [X] Add preemptive multitasking context switching to run multiple tasks at once.
- [X] Implement a simple RAM-based file system to store and load files.
- [X] Create an interface for user programs to request kernel services.
- [X] Transition from ring 0 to ring 3 execution to secure the system.
- [X] Enable VESA VBE modes for high-resolution graphics and basic drawing primitives.
- [X] Add a test harness for C utility functions to ensure stability.
- [X] Expand technical docs for driver API and memory architecture.
- [X] Clean up build system to support Linux, macOS, and Windows host build environments equally.

</details>

<details>
<summary><b>[COMPLETED] Phase 2: Core Subsystems & Persistent Storage</b></summary>

- [X] Add a PCI bus enumerator to detect, parse configuration spaces, and configure hardware.
- [X] Add support for Message Signaled Interrupts in the PCI driver.
- [X] Implement a Real-Time Clock CMOS driver for accurate system time tracking.
- [X] Upgrade timer support to the High Precision Event Timer or APIC timer.
- [X] Implement an ATA IDE PIO disk driver for basic read/write storage.
- [X] Upgrade ATA driver to support Direct Memory Access for faster disk I/O.
- [X] Create a Virtual File System abstraction layer.
- [X] Implement a FAT32 driver supporting MBR parsing, cluster tables, and Long File Names.
- [X] Implement an Ext2 file system driver with superblock, inode, and directory parsing.
- [X] Build a page cache and buffer cache mechanism to optimize disk read and write operations.
- [X] Add an ELF binary parser to dynamically load, map, and execute user-space applications.
- [X] Implement dynamic linking support for shared libraries.
- [X] Pass environment variables and command-line arguments to user programs.
- [X] Implement Unix-like Signals for basic process control.
- [X] Implement Inter-Process Communication via pipes, mailboxes, and shared memory.
- [X] Port a standard C library tailored to Nano OS system calls for user applications.

</details>

<details>
<summary><b>Phase 2.5: Modern Language Integration & Hybrid Runtime</b></summary>

- [ ] Configure the Zig build toolchain as an alternative compiler for core kernel modules.
- [ ] Establish bare-metal target specifications for Rust cross-compilation on i686 architecture.
- [ ] Integrate automated binding tools to generate compatible headers between C, C++, Rust, and Zig.
- [ ] Rewrite physical frame validation logic in Rust to guarantee spatial safety during allocation.
- [ ] Port the virtual file system path parsing routines to Zig to leverage native compile-time evaluation and error unions.
- [ ] Develop a dual-language panic runtime capable of cleanly unwinding hybrid stack frames on kernel panic.
- [ ] Provide user-space application templates and system call wrapper libraries in both Zig and Rust.

</details>

<details>
<summary><b>Phase 3: Desktop Environment & Windowing</b></summary>

- [X] Implement a PS/2 mouse driver with interrupt-driven coordinate tracking.
- [ ] Implement comprehensive keyboard mapping including modifier keys and Unicode translations.
- [ ] Build an input event queue system to route keystrokes and mouse events to the focused process.
- [ ] Implement double-buffering and hardware cursor rendering to eliminate screen tearing.
- [ ] Create a Window Server compositor architecture supporting overlapping and alpha blending.
- [ ] Implement clipping regions and dirty rectangle algorithms to optimize UI redraws.
- [ ] Add support for dragging, resizing, maximizing, and closing interactive windows.
- [ ] Develop a standard GUI Widget library with custom render loops and callback hooks.
- [ ] Implement core UI controls including buttons, text boxes, sliders, and checkboxes.
- [ ] Integrate a scalable font rendering engine for vector and bitmap typography.
- [ ] Implement anti-aliasing for text and geometric graphics primitives.
- [ ] Develop a graphical terminal emulator with ANSI escape sequences and scrollback history.
- [ ] Create a desktop shell with a functional taskbar, desktop background, and clock widget.
- [ ] Build a graphical File Explorer supporting icon view models and standard filesystem operations.
- [ ] Integrate SIMD and SSE optimized pixel blending routines for transparent window composting.
- [ ] Implement a system-wide clipboard service enabling copy and paste operations across windows.
- [ ] Add drop-shadow rendering and alpha-blended transition animations to window structures.
- [ ] Build a robust interface layout system supporting horizontal, vertical, and grid-based alignment.
- [ ] Implement sub-pixel font rendering and hinting for high-density screen resolutions.
- [ ] Develop a window manager configuration parser to define customizable hotkeys and borders.
- [ ] Add drag-and-drop protocols for transferring file paths directly between graphical application windows.
- [ ] Create a virtual desktop system allowing users to organize their working environment into multiple workspaces.
- [ ] Develop a theme parsing engine that loads color schemes and element layouts from plain text configuration files.
- [ ] Implement icon caching to optimize directory browsing in the file manager.
- [ ] Write a screen magnifier utility to improve desktop accessibility.
- [ ] Build a search-enabled graphical application launcher menu.
- [ ] Develop automated window tiling layouts including master-and-stack and spiral alignment algorithms.
- [ ] Build an image viewer application capable of decoding BMP, PNG, and TGA graphic formats.
- [ ] Develop a standard system dialogue box interface for saving, opening, and confirming files.

</details>

<details>
<summary><b>Phase 4: Networking & Advanced Peripherals</b></summary>

- [ ] Implement a loopback network interface and local routing table.
- [ ] Add basic Ethernet support using Realtek or Intel network cards with ring buffer structures.
- [ ] Build Ethernet framing and Address Resolution Protocol routing functionality.
- [ ] Implement Internet Protocol Version 4 packet routing and Ping utility functions.
- [ ] Write a Transmission Control Protocol and User Datagram Protocol state machine.
- [ ] Implement standard socket interfaces to handle network socket connections.
- [ ] Establish a USB core subsystem to handle device enumeration, descriptors, and endpoints.
- [ ] Write USB host controller drivers to communicate with legacy and high-speed USB ports.
- [ ] Implement USB Human Interface Device drivers for modern keyboards and mice.
- [ ] Add USB Mass Storage class support to read and write to flash memory drives.
- [ ] Add SATA Advanced Host Controller Interface support for modern high-speed storage disks.
- [ ] Write audio drivers to interface with legacy and high-definition sound cards.
- [ ] Implement audio stream playback using direct hardware memory access.
- [ ] Create a user-space audio daemon and mixer to combine multiple audio streams.
- [ ] Develop a Domain Name System client to resolve server addresses.
- [ ] Implement a Dynamic Host Configuration Protocol client for automatic network configuration.
- [ ] Build a basic HTTP client for retrieving remote file assets over sockets.
- [ ] Write a host controller driver to support Non-Volatile Memory Express solid-state storage.
- [ ] Implement asynchronous network event polling to handle multiple simultaneous socket files.
- [ ] Implement an Internet Control Message Protocol daemon for diagnostic route tracking.
- [ ] Write an Intel e1000 PCIe network driver leveraging packet ring descriptors.
- [ ] Develop a Realtek RTL8139 PCI network driver with transmit and receive DMA rings.
- [ ] Implement Socket Option configurations including non-blocking mode and reuse-address flags.
- [ ] Build a Network Time Protocol client to synchronize system time with global time servers.
- [ ] Develop a trivial file transfer protocol client to retrieve remote resources during boot.
- [ ] Write a USB hub driver to handle multiple devices connected to a single physical port.
- [ ] Implement a USB printer class driver to handle document output.
- [ ] Write a High Definition Audio driver with multi-channel stream support.
- [ ] Implement an audio synthesizer driver to generate simple waveform sounds.
- [ ] Develop an interface to route audio output stream data to virtual network sinks.
- [ ] Build a lightweight command-line web downloader utility.
- [ ] Add support for resolving local hostnames using multicast DNS.
- [ ] Implement TCP window scaling and selective acknowledgment to improve packet transfer rates.

</details>

<details>
<summary><b>Phase 5: Optimization, Security & System Polish</b></summary>

- [ ] Implement Symmetric Multiprocessing by parsing configuration tables and waking secondary processors.
- [ ] Add spinlocks, mutexes, and semaphores for kernel and user-space concurrency control.
- [ ] Implement a fair scheduling algorithm supporting thread affinity masks.
- [ ] Develop a Slab allocator for efficient caching of frequently used kernel objects.
- [ ] Implement demand paging and lazy loading to minimize program memory footprints.
- [ ] Enforce strict user permissions and file access control lists.
- [ ] Enable Supervisor Mode Access and Execution Prevention to protect kernel space.
- [ ] Implement No-Execute page protection on the user stack to prevent arbitrary code execution.
- [ ] Introduce Address Space Layout Randomization for user-space executable layouts.
- [ ] Create virtual filesystems to expose hardware metrics and system statistics to user-space.
- [ ] Develop a graphical System Monitor to track active processes, CPU cores, and memory usage.
- [ ] Provide comprehensive crash reporting with stack frame unwinding and register dumps.
- [ ] Add a graphical boot splash screen and secure user login console.
- [ ] Implement Kernel Address Space Layout Randomization to protect kernel memory space.
- [ ] Develop a system call auditing framework to trace and log process execution permissions.
- [ ] Create protection pages in the user-space heap with canary blocks to catch buffer overflows.
- [ ] Implement a system logging daemon that writes diagnostic events to persistent storage.
- [ ] Perform automated memory leak profiling across the virtual file system and cache structures.
- [ ] Implement a lock-free ring buffer for high-throughput message passing between processors.
- [ ] Optimize the page fault handler to handle copy-on-write page sharing.
- [ ] Implement a swap space subsystem to write inactive pages to disk under memory pressure.
- [ ] Develop an automated kernel panic recovery mechanism to restart failed non-essential services.
- [ ] Implement kernel thread pools to execute deferred background work tasks.
- [ ] Add support for the Intel SpeedStep technology to dynamically scale processor frequency.
- [ ] Build a static code analysis pass into the build system to check for memory leaks.
- [ ] Implement user resource limits to prevent runaway processes from consuming CPU and RAM.
- [ ] Add support for the hardware-based random number generator instruction.
- [ ] Develop an encrypted filesystem overlay using standard advanced encryption standards.
- [ ] Optimize context-switching code by utilizing advanced processor register state saving instructions.
- [ ] Implement a system integrity checker to verify boot binary signatures.
- [ ] Build a trace framework to profile kernel execution bottlenecks.
- [ ] Implement user group membership validation for sensitive device port access.
- [ ] Develop a micro-benchmark suite to measure context switch latency.
- [ ] Write a kernel sanitizer to detect out-of-bounds memory accesses during development.

</details>