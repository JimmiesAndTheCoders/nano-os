; ==============================================================================
; NANO OS - Interrupt Routing Stubs
; Description: Catches CPU hardware interrupts, saves the CPU state, calls our
;              C handlers, and restores the CPU state gracefully.
; ==============================================================================

[bits 32]
[extern isr_handler]
[extern irq_handler]

; ------------------------------------------------------------------------------
; ISRs (Interrupt Service Routines) - Reserved for CPU Exceptions (0-31)
; ------------------------------------------------------------------------------
%macro ISR_NOERRCODE 1
global isr%1
isr%1:
    cli
    push byte 0     ; Push dummy error code to keep stack uniform
    push byte %1    ; Push interrupt number
    jmp isr_common_stub
%endmacro

%macro ISR_ERRCODE 1
global isr%1
isr%1:
    cli
    push byte %1    ; Push interrupt number (error code is already pushed by CPU)
    jmp isr_common_stub
%endmacro

ISR_NOERRCODE 0
ISR_NOERRCODE 1
ISR_NOERRCODE 2
ISR_NOERRCODE 3
ISR_NOERRCODE 4
ISR_NOERRCODE 5
ISR_NOERRCODE 6
ISR_NOERRCODE 7
ISR_ERRCODE   8
ISR_NOERRCODE 9
ISR_ERRCODE   10
ISR_ERRCODE   11
ISR_ERRCODE   12
ISR_ERRCODE   13
ISR_ERRCODE   14
ISR_NOERRCODE 15
ISR_NOERRCODE 16
ISR_NOERRCODE 17
ISR_NOERRCODE 18
ISR_NOERRCODE 19
ISR_NOERRCODE 20
ISR_NOERRCODE 21
ISR_NOERRCODE 22
ISR_NOERRCODE 23
ISR_NOERRCODE 24
ISR_NOERRCODE 25
ISR_NOERRCODE 26
ISR_NOERRCODE 27
ISR_NOERRCODE 28
ISR_NOERRCODE 29
ISR_NOERRCODE 30
ISR_NOERRCODE 31

isr_common_stub:
    pusha               ; Pushes edi, esi, ebp, esp, ebx, edx, ecx, eax
    mov ax, ds          ; Lower 16-bits of eax = ds.
    push eax            ; Save the data segment descriptor
    mov ax, 0x10        ; Load the kernel data segment descriptor
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    push esp            ; Push a pointer to our registers_t struct for C
    call isr_handler    ; Route to our C handler!
    add esp, 4          ; Clean up the pushed esp pointer
    
    pop eax             ; Reload the original data segment descriptor
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    add esp, 8          ; Cleans up the pushed error code and pushed ISR number
    popa                ; Pops edi, esi, ebp... etc
    sti
    iret                ; Pops 5 things at once: CS, EIP, EFLAGS, SS, and ESP

; ------------------------------------------------------------------------------
; IRQs (Interrupt Requests) - Reserved for Hardware Devices (32-47)
; ------------------------------------------------------------------------------
%macro IRQ 2
global irq%1
irq%1:
    cli
    push byte 0         ; Dummy error code
    push byte %2        ; Push mapped interrupt number
    jmp irq_common_stub
%endmacro

IRQ 0, 32   ; Programmable Interval Timer (PIT)
IRQ 1, 33   ; Keyboard
IRQ 2, 34
IRQ 3, 35
IRQ 4, 36
IRQ 5, 37
IRQ 6, 38
IRQ 7, 39
IRQ 8, 40
IRQ 9, 41
IRQ 10, 42
IRQ 11, 43
IRQ 12, 44
IRQ 13, 45
IRQ 14, 46
IRQ 15, 47

irq_common_stub:
    pusha
    mov ax, ds
    push eax
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    push esp            ; Push current stack pointer (registers_t*)
    call irq_handler    ; Returns new stack pointer in EAX
    ; --- FIX: We don't 'add esp, 4' here because we want to switch to EAX ---
    mov esp, eax        ; Switch to the new stack (could be the same one or a new task)
    
    pop eax             ; Restore DS
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    add esp, 8          ; Skip int_no and err_code
    popa
    sti
    iret