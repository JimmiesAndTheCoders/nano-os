[org 0x7c00]
[bits 16]

; --- Memory Map Constants ---
KERNEL_OFFSET   equ 0x1000
PAGE_DIR_ADDR   equ 0x9000    
PAGE_TABLE_ADDR equ 0xA000
PAGE_SIZE       equ 4096

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

    ; 4. Load Kernel (17 sectors = 8.5KB)
    mov si, load_kernel_msg
    call print_string
    call load_kernel_from_disk

    ; 5. Transition to Protected Mode
    cli
    lgdt [gdt_descriptor]
    mov eax, cr0
    or eax, 0x1
    mov cr0, eax
    jmp CODE_SEG:init_pm

load_kernel_from_disk:
    pusha
    xor ax, ax
    mov es, ax
    mov bx, KERNEL_OFFSET
    mov ah, 0x02
    mov al, 17              
    mov ch, 0x00            
    mov dh, 0x00            
    mov cl, 0x02            
    mov dl, [BOOT_DRIVE]
    int 0x13
    jc disk_error
    popa
    ret

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

; ==============================================================================
; Global Descriptor Table (GDT) - FIXED SYNTAX
; ==============================================================================
gdt_start:
gdt_null:
    dd 0x0
    dd 0x0
gdt_code:
    dw 0xffff    ; Limit (bits 0-15)
    dw 0x0       ; Base (bits 0-15)
    db 0x0       ; Base (bits 16-23)
    db 10011010b ; 1st flags, type flags
    db 11001111b ; 2nd flags, Limit (bits 16-19)
    db 0x0       ; Base (bits 24-31)
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
    ; Clear memory for paging structures
    mov edi, PAGE_DIR_ADDR
    mov ecx, 2048 ; Clear Dir and one Table
    xor eax, eax
    rep stosd

    ; Identity map first 4MB
    mov edi, PAGE_TABLE_ADDR
    mov ecx, 1024
    xor eax, eax
    mov ebx, 3 ; Present + R/W
.loop:
    mov edx, eax
    or edx, ebx
    mov [edi], edx
    add edi, 4
    add eax, 4096
    loop .loop

    ; Link Dir to Table
    mov eax, PAGE_TABLE_ADDR
    or eax, 3
    mov [PAGE_DIR_ADDR], eax

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