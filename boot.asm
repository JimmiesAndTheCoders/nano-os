[org 0x7c00]
[bits 16]

; --- Memory Map Constants ---
KERNEL_OFFSET   equ 0x10000
PAGE_DIR_ADDR   equ 0x9000    
PAGE_TABLE_ADDR equ 0xA000
PAGE_SIZE       equ 4096
VBE_INFO_ADDR   equ 0x5000    ; Store VBE Mode Info here for the kernel

start:
    ; 1. Setup Segments & Stack
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    mov [BOOT_DRIVE], dl
    sti

    ; 2. Clear Screen
    mov ah, 0x00
    mov al, 0x03
    int 0x10

    mov si, boot_msg
    call print_string

    ; 3. Enable A20
    in al, 0x92
    or al, 2
    out 0x92, al

    ; 4. Load Kernel & Initrd
    mov si, load_kernel_msg
    call print_string
    call load_kernel_from_disk

    ; --- VESA VBE Setup & Initialization ---
    ; Clear VBE Info region to avoid reading stale boot memory values
    mov edi, VBE_INFO_ADDR
    xor eax, eax
    mov ecx, 64
    rep stosd

    ; Query VBE Mode Info for Mode 0x115 (800x600x24/32)
    mov ax, 0x4F01
    mov cx, 0x0115            ; FIX: Query raw mode without LFB flag (Bit 14)
    mov di, VBE_INFO_ADDR
    int 0x10
    cmp ax, 0x004F
    jne .skip_vesa            ; If query unsupported, stay in text mode

    mov ax, 0x4F02            ; Set VBE Mode
    mov bx, 0x4115            ; Keep LFB bit set here for actual mode initialization
    int 0x10
    cmp ax, 0x004F
    je .skip_vesa_clear       ; If successful, bypass text fallback

.skip_vesa:
    ; FIX: Cleanly restore VGA text mode 3 to prevent black-screen hangs on fallback
    mov ax, 0x0003
    int 0x10
    ; Force clear resolution parameters to safely enforce VGA text mode fallback
    mov word [VBE_INFO_ADDR + 18], 0

.skip_vesa_clear:
    ; 5. Transition to Protected Mode
    cli
    lgdt [gdt_descriptor]
    mov eax, cr0
    or eax, 0x1
    mov cr0, eax
    jmp CODE_SEG:init_pm

load_kernel_from_disk:
    ; Read first chunk of the kernel (120 sectors)
    mov ah, 0x42                    
    mov dl, [BOOT_DRIVE]            
    mov si, disk_address_packet1     
    int 0x13                        
    jc disk_error                   
    
    ; Read second chunk of the kernel (120 sectors)
    mov ah, 0x42                    
    mov dl, [BOOT_DRIVE]            
    mov si, disk_address_packet2     
    int 0x13                        
    jc disk_error                   
    
    ; Read Initrd (50 sectors)
    mov ah, 0x42
    mov dl, [BOOT_DRIVE]
    mov si, initrd_packet           
    int 0x13
    jc disk_error
    
    ret

align 4
disk_address_packet1:
    db 0x10                         ; Size of packet (16 bytes)
    db 0                            ; Reserved (0)
    dw 120                          ; Number of sectors to read (60 KB)
    dw 0x0000, 0x1000               ; Offset 0x0000, Segment 0x1000 (0x10000 physical)
    dq 1                            ; Starting LBA 1

disk_address_packet2:
    db 0x10                         ; Size of packet (16 bytes)
    db 0                            ; Reserved (0)
    dw 120                          ; Number of sectors to read (60 KB)
    dw 0x0000, 0x1F00               ; Offset 0x0000, Segment 0x1F00 (0x1F000 physical)
    dq 121                          ; Starting LBA 121

initrd_packet:
    db 0x10                         ; Size of packet (16 bytes)
    db 0                            ; Reserved (0)
    dw 50                           ; Number of sectors to read (25 KB)
    dw 0x0000, 0x3000               ; Offset 0x0000, Segment 0x3000 (0x30000 physical)
    dq 301                          ; Starting LBA 301

disk_error:
    mov si, error_msg
    call print_string
    cli
    hlt

print_string:
    pusha
    mov ah, 0x0E
.loop:
    lodsb
    test al, al
    jz .done
    int 0x10
    jmp .loop
.done:
    popa
    ret

; Global Descriptor Table (GDT)
gdt_start:
gdt_null:
    dd 0x0
    dd 0x0
gdt_code:
    dw 0xffff
    dw 0x0
    db 0x0
    db 10011010b
    db 11001111b
    db 0x0
gdt_data:
    dw 0xffff
    dw 0x0
    db 0x0
    db 10010010b
    db 11001111b
    db 0x0
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

CODE_SEG equ gdt_code - gdt_start
DATA_SEG equ gdt_data - gdt_start

[bits 32]
init_pm:
    mov ax, DATA_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov ebp, 0x90000
    mov esp, ebp

    call setup_paging
    jmp KERNEL_OFFSET

setup_paging:
    mov edi, PAGE_DIR_ADDR
    mov ecx, 2048
    xor eax, eax
    rep stosd

    ; Identity map first 4MB
    mov edi, PAGE_TABLE_ADDR
    mov ecx, 1024
    xor eax, eax
    mov ebx, 7 
.loop:
    mov edx, eax
    or edx, ebx
    mov [edi], edx
    add edi, 4
    add eax, 4096
    loop .loop

    mov eax, PAGE_TABLE_ADDR
    or eax, 7 
    mov [PAGE_DIR_ADDR], eax

    ; --- Map VBE Linear Framebuffer using Page Size Extension (PSE) ---
    mov eax, cr4
    or eax, 0x00000010         ; Enable PSE (Bit 4)
    mov cr4, eax

    mov eax, [VBE_INFO_ADDR + 40] ; Read Framebuffer Physical Base Address
    test eax, eax
    jz .done_paging            ; If no LFB, skip mapping

    mov edx, eax
    and edx, 0xFFC00000        ; Align to 4MB boundary
    or edx, 0x00000087         ; Present + R/W + User + 4MB Page Size Flag (Bit 7)

    mov ebx, eax
    shr ebx, 22                ; Calculate Page Directory Index
    shl ebx, 2
    add ebx, PAGE_DIR_ADDR

    mov [ebx], edx             ; Map first 4MB of video memory
    add edx, 0x400000
    mov [ebx+4], edx           ; Map second 4MB of video memory

.done_paging:
    mov eax, PAGE_DIR_ADDR
    mov cr3, eax
    mov eax, cr0
    or eax, 0x80000000
    mov cr0, eax
    ret

BOOT_DRIVE      db 0
boot_msg        db 'Nano OS Bootloader...', 0x0D, 0x0A, 0
load_kernel_msg db 'Loading Kernel...', 0x0D, 0x0A, 0
error_msg       db 'DISK ERROR!', 0

times 510-($-$$) db 0
dw 0xAA55