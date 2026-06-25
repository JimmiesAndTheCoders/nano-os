[bits 32]

global _start
extern main
extern exit

section .text

_start:
    ; Establish base pointer stack frame
    xor ebp, ebp
    push ebp
    mov ebp, esp

    ; The ELF loader (src/elf.c) configured the stack as follows:
    ; [ebp + 0]  -> Saved EBP (0)
    ; [ebp + 4]  -> Dummy return address (0)
    ; [ebp + 8]  -> argc
    ; [ebp + 12] -> argv pointer
    ; [ebp + 16] -> envp pointer

    ; Fetch and push environment variable pointer (Arg 3)
    mov eax, [ebp + 16]
    push eax

    ; Fetch and push command-line arguments array (Arg 2)
    mov eax, [ebp + 12]
    push eax

    ; Fetch and push argument count (Arg 1)
    mov eax, [ebp + 8]
    push eax

    ; Call standard C entry point
    call main

    ; Pass main's return code (EAX) directly to exit()
    push eax
    call exit

    ; Fallback infinite loop if exit system call fails
.halt:
    hlt
    jmp .halt