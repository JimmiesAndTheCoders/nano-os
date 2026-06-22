# Nano OS Driver API

This document details the interfaces available to interact with hardware components natively supported by Nano OS. 

## 1. Hardware I/O Ports
x86 hardware peripherals are largely controlled via CPU I/O ports. Nano OS uses inline GCC assembly to wrap these instructions safely (`ports.h`).

- `unsigned char port_byte_in(unsigned short port)`: Reads an 8-bit value from a specific I/O port.
- `void port_byte_out(unsigned short port, unsigned char data)`: Writes an 8-bit value to a specific I/O port.

*Example Usage:* Communicating with the Programmable Interrupt Controller (PIC) or VGA cursor registers.

## 2. Graphics & Screen Drivers

Nano OS features a dual-mode graphics subsystem: **VESA VBE (GUI)** and **VGA Text Mode (Fallback)**. The abstraction is handled by the `VgaScreen` C++ class.

### Mode Detection
During boot, the boot sector queries BIOS for VBE Mode `0x115` (800x600x24/32). The info block is placed at `0x5000`. If `vbe_info->width > 0`, the GUI driver is used; otherwise, it falls back to text memory at `0xB8000`.

### Primitive API (`graphics.h`)
- `put_pixel(x, y, color)`: Plots a single pixel on the screen using the LFB.
- `draw_rect(x, y, w, h, color)`: Draws the outline of a rectangle.
- `fill_rect(x, y, w, h, color)`: Draws a filled rectangle.
- `draw_line(x0, y0, x1, y1, color)`: Bresenham's line algorithm implementation.

### Text Rendering
Text in VBE mode is rendered using an upscaled 8x8 bitmap font array (`font8x8.h`). Standard functions like `print("string")` and `clear_screen()` automatically wrap lines and manage scrolling regardless of the active hardware mode.

## 3. Keyboard Controller
The keyboard driver (`keyboard.c`) is fully interrupt-driven, operating on IRQ 1.

- **Interrupt Translation**: `keyboard_callback()` fires whenever a key is pressed or released. It reads the scancode from port `0x60`.
- **Key Buffering**: Characters are mapped to ASCII using a static array (`sc_name`) and appended to a `key_buffer`.
- **Command Dispatch**: Pressing `ENTER` (scancode 28) clears the buffer and hands the string off to `process_command(input)` in the Nano Shell.

## 4. Programmable Interval Timer (PIT)
The PIT (`timer.c`) drives the OS heartbeat and multitasking scheduler.

- **Initialization**: `init_timer(frequency)` configures PIT channel 0 (I/O Port `0x40`) to trigger IRQ 0 at a given frequency (e.g., 100 Hz).
- **Callback & Multitasking**: Every tick increments the global time and calls `schedule(regs)`. If multiple tasks exist, the OS context-switches by changing the Stack Pointer (`ESP`) and Task State Segment (`TSS`).

## 5. System Calls (Syscalls)
Nano OS exposes a safe API to user-space applications (Ring 3) through Software Interrupts (`int 0x80`). The system call handler (`syscall.c`) acts on the `EAX` register to determine the requested action.

**Available Syscalls:**
- `SYS_PRINT` (0): Prints a null-terminated string.
- `SYS_READ_FILE` (1): Returns a memory pointer to a file from the Initrd RAM Disk.
- `SYS_LIST_FILES` (2): Outputs the list of files to the terminal.
- `SYS_GET_TICKS` (3): Returns system uptime in timer ticks.

*Note: User applications can utilize the wrapper library in `nanolib.h` to execute these interrupts cleanly.*