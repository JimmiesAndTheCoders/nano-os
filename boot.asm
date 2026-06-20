; ==============================================================================
; NANO OS - Phase 3: The Boot Sector (Fixed & Stabilized)
; Description: 16-bit bootloader that loads the C kernel from disk into memory 
;              at 0x1000, transitions to Protected Mode, and jumps to the kernel.
; ==============================================================================

[org 0x7c00]
[bits 16]

; We load our kernel to physical address 0x1000
KERNEL_OFFSET equ 0x1000

start:
    ; ------------------------------------------------------------------
    ; 1. Segment & Stack Initialization
    ; ------------------------------------------------------------------
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00          ; Stack grows downward from 0x7C00

    ; Save the boot drive number provided to us by the BIOS in DL
    mov [BOOT_DRIVE], dl
    sti

    ; ------------------------------------------------------------------
    ; 2. Clear Screen & Status message
    ; ------------------------------------------------------------------
    mov ah, 0x00
    mov al, 0x03
    int 0x10

    mov si, boot_msg
    call print_string

    ; ------------------------------------------------------------------
    ; 3. Explicitly Enable the A20 Gate (Fast A20 Method)
    ;    Ensures we can access memory above 1MB safely without wrapping.
    ; ------------------------------------------------------------------
    in al, 0x92
    or al, 2
    out 0x92, al

    ; ------------------------------------------------------------------
    ; 4. Load Kernel from Disk
    ; ------------------------------------------------------------------
    mov si, load_kernel_msg
    call print_string

    call load_kernel_from_disk

    ; ------------------------------------------------------------------
    ; 5. Transition to 32-bit Protected Mode
    ; ------------------------------------------------------------------
    cli                     ; Disable interrupts permanently before PM
    lgdt [gdt_descriptor]   ; Load GDT

    mov eax, cr0
    or eax, 0x1             ; Set PE (Protection Enable) bit
    mov cr0, eax

    ; Far jump to our 32-bit entry point, flushing 16-bit instructions
    jmp CODE_SEG:init_pm

; ==============================================================================
; Subroutine: load_kernel_from_disk
; Uses BIOS INT 13h / AH=02h to read sectors from disk into RAM.
; ==============================================================================
load_kernel_from_disk:
    pusha
    
    ; 1. Reset Disk Controller (Force BIOS to recalibrate drive heads)
    xor ax, ax              ; ah = 0x00
    mov dl, [BOOT_DRIVE]    ; Boot drive
    int 0x13
    jc disk_error           ; If carry flag is set, reset failed!

    ; 2. Read sectors from hard disk/floppy
    ; ES:BX tells the BIOS where to write the data (0x0000:0x1000)
    xor ax, ax
    mov es, ax
    mov bx, KERNEL_OFFSET

    mov ah, 0x02            ; BIOS Read Sectors function
    mov al, 50              ; INCREASED from 15 to 50 sectors
    mov ch, 0x00            ; Cylinder 0
    mov dh, 0x00            ; Head 0
    mov cl, 0x02            ; Sector 2 (Sector 1 is this bootloader!)
    mov dl, [BOOT_DRIVE]    ; Boot drive
    int 0x13                ; Call BIOS

    jc disk_error           ; Carry flag set = BIOS read error
    
    popa
    ret

disk_error:
    mov si, error_msg
    call print_string
    cli
    hlt                     ; Halt the CPU completely on disk failure

; ==============================================================================
; Real Mode Print Subroutine
; ==============================================================================
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
; Global Descriptor Table (GDT)
; ==============================================================================
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

; ==============================================================================
; Variables & Messages
; ==============================================================================
BOOT_DRIVE      db 0
boot_msg        db 'Nano OS Bootloader active...', 0x0D, 0x0A, 0
load_kernel_msg db 'Reading Kernel from disk...', 0x0D, 0x0A, 0
error_msg       db 'CRITICAL: DISK READ ERROR!', 0x0D, 0x0A, 0

; ==============================================================================
; 32-bit Protected Mode Segment
; ==============================================================================
[bits 32]
init_pm:
    mov ax, DATA_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    mov ebp, 0x90000        ; Set up stack pointer in a safe, free space
    mov esp, ebp

    ; Initialize paging and enable virtual memory with an identity map.
    call setup_paging

    ; JUMP straight to our loaded C kernel entry point!
    jmp KERNEL_OFFSET

; ==============================================================================
; Paging Setup
; ==============================================================================
PAGE_DIR_ADDR   equ 0x8000
PAGE_TABLE_ADDR equ 0x9000
PAGE_SIZE       equ 4096

setup_paging:
    push eax
    push ebx
    push ecx
    push edi

    ; Clear the page directory and page table
    mov edi, PAGE_DIR_ADDR
    mov ecx, 1024
    xor eax, eax
    rep stosd

    mov edi, PAGE_TABLE_ADDR
    mov ecx, 1024
    xor eax, eax
    rep stosd

    ; Identity-map the first 4MB of memory using 4KB pages
    mov edi, PAGE_TABLE_ADDR
    mov ecx, 1024
    xor eax, eax
    mov ebx, 3              ; Present + Read/Write flags
.page_loop:
    mov edx, eax
    or edx, ebx
    mov [edi], edx
    add edi, 4
    add eax, PAGE_SIZE
    loop .page_loop

    ; Point the first directory entry to the page table
    mov eax, PAGE_TABLE_ADDR
    or eax, 3
    mov [PAGE_DIR_ADDR], eax

    ; Load page directory base into CR3
    mov eax, PAGE_DIR_ADDR
    mov cr3, eax

    ; Enable paging by setting the PG bit in CR0
    mov eax, cr0
    or eax, 0x80000000
    mov cr0, eax

    pop edi
    pop ecx
    pop ebx
    pop eax
    ret

; ==============================================================================
; Padding and Boot Signature
; ==============================================================================
times 510-($-$$) db 0
dw 0xAA55