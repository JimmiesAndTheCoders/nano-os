#include "timer.h"
#include "ports.h"
#include "cpu.h"

static unsigned int timer_ticks = 0;

void timer_callback(registers_t *regs) {
    (void)regs;
    timer_ticks++;
}

void init_timer(unsigned int frequency) {
    unsigned int divisor = 1193180 / frequency;

    /* Register the timer IRQ handler at IRQ0 (interrupt 32) */
    extern void register_interrupt_handler(unsigned char, void (*)(registers_t*));
    register_interrupt_handler(32, timer_callback);

    port_byte_out(0x43, 0x36);          /* Command byte: channel 0, low/high byte, mode 3 */
    port_byte_out(0x40, divisor & 0xFF);
    port_byte_out(0x40, (divisor >> 8) & 0xFF);
}

unsigned int timer_get_ticks() {
    return timer_ticks;
}
