#include "cpu.h"
#include "util.h"
#include "ports.h"
#include "screen.h"

#define IDT_ENTRIES 256
idt_gate_t idt[IDT_ENTRIES];
idt_register_t idt_reg;

/* Array of function pointers to hold our custom C handlers */
isr_t interrupt_handlers[256];

/* We declare the external assembly labels from interrupt.asm */
extern void isr0(); extern void isr1(); extern void isr2(); extern void isr3();
extern void isr4(); extern void isr5(); extern void isr6(); extern void isr7();
extern void isr8(); extern void isr9(); extern void isr10(); extern void isr11();
extern void isr12(); extern void isr13(); extern void isr14(); extern void isr15();
extern void isr16(); extern void isr17(); extern void isr18(); extern void isr19();
extern void isr20(); extern void isr21(); extern void isr22(); extern void isr23();
extern void isr24(); extern void isr25(); extern void isr26(); extern void isr27();
extern void isr28(); extern void isr29(); extern void isr30(); extern void isr31();

extern void irq0(); extern void irq1(); extern void irq2(); extern void irq3();
extern void irq4(); extern void irq5(); extern void irq6(); extern void irq7();
extern void irq8(); extern void irq9(); extern void irq10(); extern void irq11();
extern void irq12(); extern void irq13(); extern void irq14(); extern void irq15();

void set_idt_gate(int n, unsigned int handler) {
	idt[n].base_low = low_16(handler);
	idt[n].sel = 0x08; /* KERNEL_CS */
	idt[n].always0 = 0;
	idt[n].flags = 0x8E; 
	idt[n].base_high = high_16(handler);
}

void set_idt() {
	idt_reg.base = (unsigned int) &idt;
	idt_reg.limit = IDT_ENTRIES * sizeof(idt_gate_t) - 1;
	__asm__ __volatile__("lidtl (%0)" : : "r" (&idt_reg));
}

void isr_install() {
	set_idt_gate(0, (unsigned int)isr0); set_idt_gate(1, (unsigned int)isr1);
	set_idt_gate(2, (unsigned int)isr2); set_idt_gate(3, (unsigned int)isr3);
	set_idt_gate(4, (unsigned int)isr4); set_idt_gate(5, (unsigned int)isr5);
	set_idt_gate(6, (unsigned int)isr6); set_idt_gate(7, (unsigned int)isr7);
	set_idt_gate(8, (unsigned int)isr8); set_idt_gate(9, (unsigned int)isr9);
	set_idt_gate(10, (unsigned int)isr10); set_idt_gate(11, (unsigned int)isr11);
	set_idt_gate(12, (unsigned int)isr12); set_idt_gate(13, (unsigned int)isr13);
	set_idt_gate(14, (unsigned int)isr14); set_idt_gate(15, (unsigned int)isr15);
	set_idt_gate(16, (unsigned int)isr16); set_idt_gate(17, (unsigned int)isr17);
	set_idt_gate(18, (unsigned int)isr18); set_idt_gate(19, (unsigned int)isr19);
	set_idt_gate(20, (unsigned int)isr20); set_idt_gate(21, (unsigned int)isr21);
	set_idt_gate(22, (unsigned int)isr22); set_idt_gate(23, (unsigned int)isr23);
	set_idt_gate(24, (unsigned int)isr24); set_idt_gate(25, (unsigned int)isr25);
	set_idt_gate(26, (unsigned int)isr26); set_idt_gate(27, (unsigned int)isr27);
	set_idt_gate(28, (unsigned int)isr28); set_idt_gate(29, (unsigned int)isr29);
	set_idt_gate(30, (unsigned int)isr30); set_idt_gate(31, (unsigned int)isr31);

	set_idt(); 
}

void irq_install() {
	/* Remap the PIC (Programmable Interrupt Controller) so IRQs don't conflict with CPU Exceptions */
	port_byte_out(0x20, 0x11); port_byte_out(0xA0, 0x11);
	port_byte_out(0x21, 0x20); port_byte_out(0xA1, 0x28);
	port_byte_out(0x21, 0x04); port_byte_out(0xA1, 0x02);
	port_byte_out(0x21, 0x01); port_byte_out(0xA1, 0x01);
	port_byte_out(0x21, 0x0);  port_byte_out(0xA1, 0x0);

	/* Install the IRQs into the IDT at mapped offsets 32-47 */
	set_idt_gate(32, (unsigned int)irq0); set_idt_gate(33, (unsigned int)irq1);
	set_idt_gate(34, (unsigned int)irq2); set_idt_gate(35, (unsigned int)irq3);
	set_idt_gate(36, (unsigned int)irq4); set_idt_gate(37, (unsigned int)irq5);
	set_idt_gate(38, (unsigned int)irq6); set_idt_gate(39, (unsigned int)irq7);
	set_idt_gate(40, (unsigned int)irq8); set_idt_gate(41, (unsigned int)irq9);
	set_idt_gate(42, (unsigned int)irq10); set_idt_gate(43, (unsigned int)irq11);
	set_idt_gate(44, (unsigned int)irq12); set_idt_gate(45, (unsigned int)irq13);
	set_idt_gate(46, (unsigned int)irq14); set_idt_gate(47, (unsigned int)irq15);

	set_idt();
}

void register_interrupt_handler(unsigned char n, isr_t handler) {
	interrupt_handlers[n] = handler;
}

void isr_handler(registers_t *r) {
	print("CPU EXCEPTION! System Halted.\n");
	__asm__("hlt");
}

void irq_handler(registers_t *r) {
	/* Send an EOI (End of Interrupt) signal to the PICs so they know we handled it */
	if (r->int_no >= 40) port_byte_out(0xA0, 0x20); /* Slave */
	port_byte_out(0x20, 0x20);  

	/* Handle the interrupt in C if we registered a callback */
	if (interrupt_handlers[r->int_no] != 0) {
		isr_t handler = interrupt_handlers[r->int_no];
		handler(r);
	}
}
