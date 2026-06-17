# ==============================================================================
# NANO OS - Organized Makefile
# Description: Uses src/ and include/ directories for a clean workspace.
# ==============================================================================

ASM      = nasm
CC       = i686-elf-gcc
LD       = i686-elf-ld
VBOX     = VBoxManage

# Directories
BUILD_DIR = build
SRC_DIR   = src
INC_DIR   = include

# Output filenames
BOOT_BIN = $(BUILD_DIR)/boot.bin
ENTRY_O  = $(BUILD_DIR)/kernel_entry.o
INTR_O   = $(BUILD_DIR)/interrupt.o
KERN_BIN = $(BUILD_DIR)/kernel.bin
RAW_IMG  = $(BUILD_DIR)/nano_os.img
VDI_IMG  = nano_os.vdi
TARGET_UUID = d74d67a5-9c49-432e-856f-503a1abae2d8

# Compiler flags: add the include directory
CFLAGS = -ffreestanding -fno-pie -fno-stack-protector -nostdlib -I$(INC_DIR)

# Find sources
C_SOURCES = $(wildcard $(SRC_DIR)/*.c)
C_OBJS    = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(C_SOURCES))

all: $(VDI_IMG)

# Setup directories
$(BUILD_DIR):
	@mkdir $(BUILD_DIR) 2>nul || mkdir -p $(BUILD_DIR) 2>nul || true

# Bootloader
$(BOOT_BIN): boot.asm | $(BUILD_DIR)
	$(ASM) -f bin boot.asm -o $(BOOT_BIN)

# Kernel Entry
$(ENTRY_O): $(SRC_DIR)/kernel_entry.asm | $(BUILD_DIR)
	$(ASM) -f elf32 $(SRC_DIR)/kernel_entry.asm -o $(ENTRY_O)

# Interrupts
$(INTR_O): $(SRC_DIR)/interrupt.asm | $(BUILD_DIR)
	$(ASM) -f elf32 $(SRC_DIR)/interrupt.asm -o $(INTR_O)

# C files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Linking
$(KERN_BIN): $(ENTRY_O) $(INTR_O) $(C_OBJS) linker.ld | $(BUILD_DIR)
	$(LD) -T linker.ld $(ENTRY_O) $(INTR_O) $(C_OBJS) -o $(KERN_BIN)

# Image creation
$(RAW_IMG): $(BOOT_BIN) $(KERN_BIN) | $(BUILD_DIR)
	cat $(BOOT_BIN) $(KERN_BIN) > $(BUILD_DIR)/nano_os_raw.bin
	dd if=/dev/zero of=$(RAW_IMG) bs=512 count=2880
	dd if=$(BUILD_DIR)/nano_os_raw.bin of=$(RAW_IMG) conv=notrunc

# VDI Deployment
$(VDI_IMG): $(RAW_IMG)
	-$(VBOX) closemedium disk $(VDI_IMG) 2>/dev/null || true
	rm -f $(VDI_IMG)
	$(VBOX) convertfromraw $(RAW_IMG) $(VDI_IMG) --format VDI
	$(VBOX) internalcommands sethduuid $(VDI_IMG) $(TARGET_UUID)

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean