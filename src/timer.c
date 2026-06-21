#include "timer.h"
#include "ports.h"
#include "cpu.h"
#include "task.h"

static unsigned int timer_ticks = 0;

// Update to return unsigned int
unsigned int timer_callback(registers_t *regs) {
    timer_ticks++; // DON'T FORGET THIS
    return schedule(regs);
}

void init_timer(unsigned int frequency) {
    unsigned int divisor = 1193180 / frequency;

    // Remove the manual 'extern' line that was causing the error
    register_interrupt_handler(32, timer_callback);

    port_byte_out(0x43, 0x36);
    port_byte_out(0x40, divisor & 0xFF);
    port_byte_out(0x40, (divisor >> 8) & 0xFF);
}

unsigned int timer_get_ticks() {
    return timer_ticks;
}