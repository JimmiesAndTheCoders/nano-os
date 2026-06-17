/* ==============================================================================
 * NANO OS - Linker Script (Fixed Alignment & Paths)
 * Description: Instructs the linker how to build our kernel binary. We force
 * the kernel_entry code to be positioned at the absolute start and align sections.
 * ============================================================================== */

ENTRY(_start)
OUTPUT_FORMAT("binary")

SECTIONS
{
    /* Our kernel is loaded at address 0x1000 in RAM */
    . = 0x1000;

    .text : ALIGN(4) {
        /* Force kernel_entry.o to be at the very front of the text section.
         * The asterisk '*' allows us to match the file even when compiled inside 
         * subdirectories like 'build/kernel_entry.o'. */
        *kernel_entry.o(.text)
        *(.text)
    }

    .data : ALIGN(4) {
        *(.data)
    }

    .bss : ALIGN(4) {
        *(.bss)
        *(COMMON)
    }
}