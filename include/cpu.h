#ifndef CPU_H
#define CPU_H

/* Macros to extract the top and bottom 16 bits of a 32-bit memory address. 
 * Placed directly here to ensure they are always available during IDT configuration. */
#define low_16(address) (unsigned short)((address) & 0xFFFF)
#define high_16(address) (unsigned short)(((address) >> 16) & 0xFFFF)

/* IDT Gate definition */
typedef struct {
    unsigned short base_low;   /* Lower 16 bits of handler function address */
    unsigned short sel;        /* Kernel segment selector */
    unsigned char always0;
    unsigned char flags;       /* Flags: ring 0, present, etc. */
    unsigned short base_high;  /* Higher 16 bits of handler function address */
} __attribute__((packed)) idt_gate_t;

/* IDT Pointer definition */
typedef struct {
    unsigned short limit;
    unsigned int base;
} __attribute__((packed)) idt_register_t;

/* This struct exactly matches how 'pusha' and our asm stubs push data onto the stack */
typedef struct {
    unsigned int ds;
    unsigned int edi, esi, ebp, esp, ebx, edx, ecx, eax;
    unsigned int int_no, err_code;
    unsigned int eip, cs, eflags, useresp, ss;
} registers_t;

void isr_install();
void irq_install();

unsigned int irq_handler(registers_t *r);
void isr_handler(registers_t *r);

typedef void (*isr_t)(registers_t*);
void register_interrupt_handler(unsigned char n, isr_t handler);

#endif