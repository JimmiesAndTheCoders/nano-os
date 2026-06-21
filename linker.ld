ENTRY(_start)
OUTPUT_FORMAT("binary")

SECTIONS
{
    . = 0x1000;

    .text : ALIGN(4) {
        *kernel_entry.o(.text)
        *(.text)
        *(.rodata)    /* Move strings into the text segment for safety */
        *(.rodata.*)
    }

    .data : ALIGN(4) {
        *(.data)
    }

    .bss : ALIGN(4) {
        *(.bss)
        *(COMMON)
    }

    /* Move constructors here */
    .init_array : ALIGN(4) {
        _start_ctors = .;
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array))
        _end_ctors = .;
    }
}