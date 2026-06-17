; ==============================================================================
; NANO OS - Kernel Assembly Entry Point
; Description: Linked at the very beginning of the kernel binary. It guarantees
;              that the CPU jumps immediately into our C kernel code.
; ==============================================================================

[bits 32]
section .text          ; CRITICAL: Explicitly define the .text section so the 
                       ; linker script can position this file at the absolute front!

[extern kernel_main]   ; kernel_main is defined in our C file (kernel.c)

global _start
_start:
    call kernel_main   ; Jump to the C entry point!
    jmp $              ; If the kernel returns for any reason, halt the CPU in an infinite loop