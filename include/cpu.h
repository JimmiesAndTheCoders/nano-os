#ifndef CPU_H
#define CPU_H

/* Macros to extract the top and bottom 16 bits of a 32-bit memory address. */
#define low_16(address) (unsigned short)((address) & 0xFFFF)
#define high_16(address) (unsigned short)(((address) >> 16) & 0xFFFF)

/* IDT structures */
typedef struct {
    unsigned short base_low;
    unsigned short sel;
    unsigned char always0;
    unsigned char flags;
    unsigned short base_high;
} __attribute__((packed)) idt_gate_t;

typedef struct {
    unsigned short limit;
    unsigned int base;
} __attribute__((packed)) idt_register_t;

/* This struct matches the stack layout during an interrupt */
typedef struct {
    unsigned int ds;
    unsigned int edi, esi, ebp, esp, ebx, edx, ecx, eax;
    unsigned int int_no, err_code;
    unsigned int eip, cs, eflags; 
} registers_t;

/* --- CRITICAL: The definition of isr_t must be here --- */
typedef unsigned int (*isr_t)(registers_t*);

/* Function Prototypes */
void isr_install();
void irq_install();
unsigned int isr_handler(registers_t *r);
unsigned int irq_handler(registers_t *r);
void register_interrupt_handler(unsigned char n, isr_t handler);

#endif