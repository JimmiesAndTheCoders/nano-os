# Nano OS

Nano OS is a lightweight, 32-bit hobby operating system developed from scratch (well, not from scratch, it was made with AI or something like that). It features a custom bootloader, a protected-mode C kernel, VGA hardware drivers, interrupt management, and an interactive command-line shell.

## Key Features

- **Custom Bootloader**: Minimalist 16-bit real-mode assembly boot sector.
- **32-bit Protected Mode**: Seamless transition to high-performance C execution.
- **Hardware Drivers**: VGA text-mode driver with screen scrolling and cursor control.
- **Interrupt Management**: Full Interrupt Descriptor Table (IDT) implementation for hardware/CPU event handling.
- **Keyboard Driver**: Real-time hardware interrupt-driven keyboard input with scancode-to-ASCII translation.
- **Nano Shell**: Built-in CLI with command parsing, backspace/newline support, and custom command execution.
- **Automated Build System**: Professional Makefile-driven compilation with clean, organized directory structures.

## Build & Run Instructions

### Prerequisites

- `nasm`: Assembler
- `i686-elf-gcc` / `i686-elf-ld`: Cross-compiler toolchain
- `make`: Build automation
- Almost any virtual machine (e.g., QEMU, VirtualBox) or emulator (e.g., Bochs) that supports booting from a disk image.

### Building

To clone the repository and build the OS, follow these steps:

```bash
git clone https://github.com/JimmiesAndTheCoders/nano-os.git
cd nano-os
```

Then, to build the OS, simply run:

```bash
make clean && make
```

### Running

#### Using VirtualBox

The build system automatically compiles the VDI image and applies the target UUID configured in your Makefile.

1. Open VirtualBox.
2. Create a new virtual machine (e.g., "Nano OS") with the following settings:
   - Type: Other
   - Version: Other/Unknown (32-bit)
   - Memory: 512 MB
   - Hard Disk: Use an existing virtual hard disk file → Select `build/nano-os.vdi`
3. Start the virtual machine to boot into Nano OS.

> Note: If the mouse is captured, press the Host key to release it.

#### Using QEMU (Recommended for testing)

QEMU is highly recommended for OS development because it boots raw files directly and has no disk cache locks. To run Nano OS with QEMU, use the following command:

```bash
qemu-system-i386 -drive format=raw,file=build/nano_os.img
```

#### Using VMware

VMware supports the Virtual Machine Disk (.vmdk) format. You can convert the raw image file to VMDK format:

```bash
# Convert raw image to VMware format
VBoxManage convertfromraw build/nano_os.img nano_os.vmdk --format VMDK
```

Once you have the VMDK file, follow these steps:

1. Open **VMware Workstation** or **VMware Player**.
2. Click on **Create a New Virtual Machine** (or press `Ctrl+N`).
3. Select **Custom (advanced)** and click **Next**.
4. Leave the hardware compatibility at the default highest version and click **Next**.
5. Select **I will install the operating system later** and click **Next**.
6. For the guest operating system, select **Other** and choose **Other 32-bit**. Click **Next**.
7. Name your virtual machine (e.g., "Nano OS").
8. For processors, 1 CPU and 1 core is more than enough. Click **Next**.
9. Assign 64 MB to 128 MB of RAM and click **Next**.
10. For network type, you can select **Do not use a network connection** since we won't be using network features.
11. For the I/O controller type, select **LSI Logic**.
12. On the Select a Disk page, choose **Use an existing virtual disk** and click **Next**.
13. Browse to the converted `nano_os.vmdk` file and select it.
14. When you click Next, VMware may prompt you to convert the disk to the latest format. Choose "Keep existing format" to avoid unnecessary conversions.
15. Click Finish to create the virtual machine.

#### Using Hyper-V

Hyper-V requires a Virtual Hard Disk (`.vhdx`) format. You can convert your raw image file to VHDX format:

```bash
# Convert raw image to Hyper-V format
VBoxManage convertfromraw build/nano_os.img nano_os.vhdx --format VHDX
```

Once you have the VHDX file, follow these steps:

1. Open Hyper-V Manager.
2. In the right-hand Actions panel, click New -> Virtual Machine.
3. Click Next and give your VM a name (e.g., "Nano OS").
4. Select Generation 1 (Generation 2 only boots UEFI; we are booting legacy BIOS).
5. Assign 64 MB to 128 MB of RAM.
6. Uncheck "Use Dynamic Memory for this virtual machine", and click Next.
7. For a custom hobby OS, you can usually leave this on **Not Connected** since we won't be using network features.
8. Select "Use an existing virtual hard disk" and browse to the converted `nano_os.vhdx` file. Click Next.
9. Click Finish to create the virtual machine.
10. Right-click the new VM, select Connect, and then click Start to boot into Nano OS.

#### Using Bochs

Bochs is a highly configurable x86 emulator. To run Nano OS with Bochs, use the following command:

```bash
bochs -f bochsrc
```

## Using Nano Shell

Once booted, interact with the system using the following commands:

- `help`: Displays a list of available commands.
- `clear`: Clears the screen.
- `status`: Displays kernel operational metrics.
- `nano --status`: View system mascot information.
- `halt`: Safely stops the CPU.

> Note: We'll be adding more commands in the future, so stay tuned!

## Roadmap

Check out the [Roadmap](ROADMAP.md) for upcoming features and improvements.
