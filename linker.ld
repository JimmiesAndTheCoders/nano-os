ENTRY(_start)
OUTPUT_FORMAT("binary")

SECTIONS
{
    . = 0x1000;

    .text : ALIGN(4) {
        *kernel_entry.o(.text)
        *(.text)
    }

    .rodata : ALIGN(4) {
        *(.rodata)
        *(.rodata.*)
        
        _start_ctors = .;
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array))
        _end_ctors = .;
    }

    .data : ALIGN(4) {
        *(.data)
    }

    .bss : ALIGN(4) {
        *(.bss)
        *(COMMON)
    }
}