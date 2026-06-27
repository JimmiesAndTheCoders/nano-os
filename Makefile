# ==============================================================================
# NANO OS - Clean Unified Makefile (with Zig Compiler Integration)
# ==============================================================================

ASM      = nasm
CC       = i686-elf-gcc
CPP      = i686-elf-g++
LD       = i686-elf-ld
AR       = i686-elf-ar
ZIG      = zig
VBOX     = VBoxManage
HOST_CC  = gcc
NCC = $(BUILD_DIR)/ncc$(HOST_EXE)

ifeq ($(OS),Windows_NT)
    HOST_EXE = .exe
else
    HOST_EXE =
endif

# Directories
BUILD_DIR = build
SRC_DIR   = src
INC_DIR   = include
TEST_DIR  = tests
TOOLS_DIR = tools
LIBC_DIR  = libc

# Output filenames
BOOT_BIN        = $(BUILD_DIR)/boot.bin
ENTRY_O         = $(BUILD_DIR)/kernel_entry.o
INTR_O          = $(BUILD_DIR)/interrupt.o
KERN_BIN        = $(BUILD_DIR)/kernel.bin
FULL_KERN       = $(BUILD_DIR)/full_kernel.bin
RAW_IMG         = $(BUILD_DIR)/nano_os.img
VDI_IMG         = nano_os.vdi
INITRD_IMG      = $(BUILD_DIR)/initrd.img
USER_ELF        = $(BUILD_DIR)/user_hello.elf
USER_MALLOC_ELF = $(BUILD_DIR)/user_malloc_test.elf
USER_PRIME_ELF  = $(BUILD_DIR)/user_prime.elf
USER_IPC_ELF    = $(BUILD_DIR)/user_ipc_demo.elf
LIBC_A          = $(BUILD_DIR)/libnano.a
CRT0_O          = $(BUILD_DIR)/crt0.o
TARGET_UUID     = d74d67a5-9c49-432e-856f-503a1abae2d8

# Host Tools and Tests
MAKE_INITRD = $(BUILD_DIR)/make_initrd$(HOST_EXE)
TEST_BIN    = $(BUILD_DIR)/test_util$(HOST_EXE)

# Compiler flags
CFLAGS      = -ffreestanding -fno-pic -fno-pie -fno-stack-protector -nostdlib -I$(INC_DIR)
CPPFLAGS    = -ffreestanding -fno-pic -fno-pie -O2 -Wall -Wextra -fno-exceptions -fno-rtti -I$(INC_DIR)
HOST_CFLAGS = -I$(INC_DIR) -Wall -Wextra -g -fno-builtin

# Zig Compiler flags (freestanding 32-bit x86 target)
ZIGFLAGS    = build-obj -target x86-freestanding-none -O ReleaseSafe

# Libc Compiler flags
LIBC_INC_DIR = $(LIBC_DIR)/include
LIBC_CFLAGS  = -ffreestanding -fno-pic -fno-pie -fno-stack-protector -nostdlib -m32 -I$(LIBC_INC_DIR) -Wall -Wextra

# Find all C, C++, and Zig sources
C_SOURCES     = $(wildcard $(SRC_DIR)/*.c)
CPP_SOURCES   = $(wildcard $(SRC_DIR)/*.cpp)
ZIG_SOURCES   = $(wildcard $(SRC_DIR)/*.zig)
SHELL_SOURCES = $(wildcard $(SRC_DIR)/shell/*.c)
LIBC_SOURCES  = $(wildcard $(LIBC_DIR)/src/*.c)

# Convert sources to object file paths in build/
C_OBJS       = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(C_SOURCES))
CPP_OBJS     = $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(CPP_SOURCES))
ZIG_OBJS     = $(patsubst $(SRC_DIR)/%.zig, $(BUILD_DIR)/%.o, $(ZIG_SOURCES))
LIBC_OBJS    = $(patsubst $(LIBC_DIR)/src/%.c, $(BUILD_DIR)/libc_%.o, $(LIBC_SOURCES))
SHELL_OBJS   = $(patsubst $(SRC_DIR)/shell/%.c, $(BUILD_DIR)/shell_%.o, $(SHELL_SOURCES))
ALL_OBJS     = $(C_OBJS) $(CPP_OBJS) $(ZIG_OBJS) $(SHELL_OBJS)

all: $(VDI_IMG)

# Setup directories
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Compilation of libc assembly startup helper (CRT0)
$(CRT0_O): $(LIBC_DIR)/src/crt0.asm | $(BUILD_DIR)
	$(ASM) -f elf32 $< -o $@

# Compilation of libc object files
$(BUILD_DIR)/libc_%.o: $(LIBC_DIR)/src/%.c | $(BUILD_DIR)
	$(CC) $(LIBC_CFLAGS) -c $< -o $@

# Compile files from the split shell subfolder
$(BUILD_DIR)/shell_%.o: $(SRC_DIR)/shell/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Archiving libc into static library
$(LIBC_A): $(LIBC_OBJS)
	$(AR) rcs $(LIBC_A) $(LIBC_OBJS)

# Build the ncc host tool
$(NCC): $(TOOLS_DIR)/ncc.c | $(BUILD_DIR)
	$(HOST_CC) $(HOST_CFLAGS) $(TOOLS_DIR)/ncc.c -o $(NCC)

# Compile user_hello.elf using ncc
$(USER_ELF): tests/user_hello.c $(CRT0_O) $(LIBC_A) $(NCC)
	$(NCC) tests/user_hello.c $(USER_ELF)

# Compile user_malloc_test.elf using ncc
$(USER_MALLOC_ELF): tests/user_malloc_test.c $(CRT0_O) $(LIBC_A) $(NCC)
	$(NCC) tests/user_malloc_test.c $(USER_MALLOC_ELF)

# Compile user_prime.elf using ncc
$(USER_PRIME_ELF): tests/user_prime.c $(CRT0_O) $(LIBC_A) $(NCC)
	$(NCC) tests/user_prime.c $(USER_PRIME_ELF)

# Compile user_ipc_demo.elf using ncc
$(USER_IPC_ELF): tests/user_ipc_demo.c $(CRT0_O) $(LIBC_A) $(NCC)
	$(NCC) tests/user_ipc_demo.c $(USER_IPC_ELF)

# Bootloader
$(BOOT_BIN): boot.asm | $(BUILD_DIR)
	$(ASM) -f bin boot.asm -o $(BOOT_BIN)

# Kernel Entry
$(ENTRY_O): $(SRC_DIR)/kernel_entry.asm | $(BUILD_DIR)
	$(ASM) -f elf32 $(SRC_DIR)/kernel_entry.asm -o $(ENTRY_O)

# Interrupts
$(INTR_O): $(SRC_DIR)/interrupt.asm | $(BUILD_DIR)
	$(ASM) -f elf32 $(SRC_DIR)/interrupt.asm -o $(INTR_O)

# C and C++ files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@
    
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	$(CPP) $(CPPFLAGS) -c $< -o $@

# Zig source compilation
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.zig | $(BUILD_DIR)
	$(ZIG) $(ZIGFLAGS) $< -femit-bin=$@

# Tool build
$(MAKE_INITRD): $(TOOLS_DIR)/make_initrd.c | $(BUILD_DIR)
	$(HOST_CC) $(TOOLS_DIR)/make_initrd.c -o $(MAKE_INITRD)

# Generate the file system image containing all ELF binaries
$(INITRD_IMG): $(MAKE_INITRD) $(USER_ELF) $(USER_MALLOC_ELF) $(USER_PRIME_ELF) $(USER_IPC_ELF) | $(BUILD_DIR)
	$(MAKE_INITRD) $(INITRD_IMG) $(USER_ELF) $(USER_MALLOC_ELF) $(USER_PRIME_ELF) $(USER_IPC_ELF)

# Linking
$(KERN_BIN): $(ENTRY_O) $(INTR_O) $(ALL_OBJS) linker.ld | $(BUILD_DIR)
	$(LD) -T linker.ld $(ENTRY_O) $(INTR_O) $(ALL_OBJS) -o $(KERN_BIN)

# Image creation
$(RAW_IMG): $(BOOT_BIN) $(KERN_BIN) $(INITRD_IMG) | $(BUILD_DIR)
	cat $(BOOT_BIN) $(KERN_BIN) > $(FULL_KERN)
	dd if=/dev/zero of=$(RAW_IMG) bs=512 count=20480
	dd if=$(FULL_KERN) of=$(RAW_IMG) conv=notrunc
	dd if=$(INITRD_IMG) of=$(RAW_IMG) obs=512 seek=601 conv=notrunc

# VDI Deployment
$(VDI_IMG): $(RAW_IMG)
	-$(VBOX) closemedium disk $(VDI_IMG) 2>/dev/null || true
	rm -f $(VDI_IMG)
	$(VBOX) convertfromraw $(RAW_IMG) $(VDI_IMG) --format VDI
	$(VBOX) internalcommands sethduuid $(VDI_IMG) $(TARGET_UUID)

# Host Test Harness
test: $(TEST_DIR)/test_util.c $(SRC_DIR)/util.c | $(BUILD_DIR)
	$(HOST_CC) $(HOST_CFLAGS) $(TEST_DIR)/test_util.c $(SRC_DIR)/util.c -o $(TEST_BIN)
	@printf "\n=== Running Unit Tests ===\n"
	@./$(TEST_BIN)

clean:
	rm -rf $(BUILD_DIR)
	rm -f $(VDI_IMG)

.PHONY: all clean test
