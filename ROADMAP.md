# Nano OS Development Roadmap

This roadmap outlines the planned features and improvements for Nano OS. The project is in active development, and we welcome contributions from the community. Below are the key milestones and features we aim to implement in the coming months.

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
- [ ] Clean up build system to support Linux, macOS, and Windows host build environments equally.
- [ ] Implement a simple graphical user interface (GUI) with mouse support.
- [ ] Add networking support (e.g., Ethernet driver, TCP/IP stack) to enable internet connectivity.
- [ ] Add support for loading and running user-space applications.
- [ ] Implement a basic shell with command history and scripting capabilities.
- [ ] Add support for additional hardware devices (e.g., disk drives, network cards).
- [ ] Optimize the kernel for performance and stability.

This roadmap is subject to change as the project evolves, but it provides a clear direction for the development of Nano OS. We encourage contributors to pick any of the open tasks and help us bring Nano OS to life!
