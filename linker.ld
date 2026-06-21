ENTRY(_start)
OUTPUT_FORMAT("binary")

SECTIONS
{
    . = 0x10000;   /* <-- CHANGE: Shift this up from 0x1000 to 0x10000 */

    .text : ALIGN(4) {
        *kernel_entry.o(.text)
        *(.text)
        *(.rodata)    
        *(.rodata.*)
    }

    .data : ALIGN(4) {
        *(.data)
    }

    .bss : ALIGN(4) {
        *(.bss)
        *(COMMON)
    }

    .init_array : ALIGN(4) {
        _start_ctors = .;
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array))
        _end_ctors = .;
    }
}