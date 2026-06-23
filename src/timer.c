#include "timer.h"
#include "ports.h"
#include "cpu.h"
#include "task.h"

static unsigned int timer_ticks = 0;
static int is_apic_active = 0;

/* Local APIC Register Address Offsets */
#define LAPIC_R_TIMER_LVT      ((volatile unsigned int*)0xFEE00320)
#define LAPIC_R_TIMER_INIT_CNT ((volatile unsigned int*)0xFEE00380)
#define LAPIC_R_TIMER_CUR_CNT  ((volatile unsigned int*)0xFEE00390)
#define LAPIC_R_TIMER_DIV      ((volatile unsigned int*)0xFEE003E0)

unsigned int timer_callback(registers_t *regs) {
    timer_ticks++;
    return schedule(regs);
}

void init_timer(unsigned int frequency) {
    unsigned int divisor = 1193180 / frequency;
    register_interrupt_handler(32, timer_callback);

    port_byte_out(0x43, 0x36);
    port_byte_out(0x40, divisor & 0xFF);
    port_byte_out(0x40, (divisor >> 8) & 0xFF);
}

void init_apic_timer(unsigned int frequency) {
    if (frequency == 0) return;

    // Verify if LAPIC is enabled and mapped in the page table directory
    unsigned int* pd = (unsigned int*)0x9000;
    if (!(pd[0xFEE00000 >> 22] & 1)) {
        is_apic_active = 0;
        return;
    }

    // Calibrate LAPIC Timer using PIT Channel 2
    // Enable PIT Channel 2 gate (Bit 0), disable speaker data (Bit 1)
    unsigned char port_61 = port_byte_in(0x61);
    port_byte_out(0x61, (port_61 & 0xFD) | 0x01);

    // Configure Channel 2: Mode 0 (Interrupt on Terminal Count), LSB/MSB
    port_byte_out(0x43, 0xB0);

    // Load reload value for 10ms calibration window (11932 PIT ticks)
    port_byte_out(0x42, 0x9C); // LSB
    port_byte_out(0x42, 0x2E); // MSB

    // Set LAPIC Divider to 16 (register value 0x3)
    *LAPIC_R_TIMER_DIV = 0x3;
    *LAPIC_R_TIMER_INIT_CNT = 0xFFFFFFFF;

    // Poll PIT Channel 2 Output Status (Bit 5 of port 0x61) until countdown finishes
    int timeout = 1000000;
    while (!(port_byte_in(0x61) & 0x20)) {
        if (--timeout == 0) {
            // Calibration timed out; safely restore gate state and fallback
            port_byte_out(0x61, port_byte_in(0x61) & 0xFC);
            is_apic_active = 0;
            return;
        }
    }

    unsigned int ticks_in_10ms = 0xFFFFFFFF - *LAPIC_R_TIMER_CUR_CNT;

    // Restore original PIT Channel 2 gate status
    port_byte_out(0x61, port_byte_in(0x61) & 0xFC);

    if (ticks_in_10ms == 0 || ticks_in_10ms == 0xFFFFFFFF) {
        is_apic_active = 0;
        return;
    }

    // Calculate APIC reload count matching the requested frequency
    // (ticks_in_10ms represents 100 Hz period duration)
    unsigned int ticks_per_period = (ticks_in_10ms * 100) / frequency;

    // Disable legacy PIT Channel 0 interrupts on Master PIC to prevent overlapping ticks
    unsigned char pic_mask = port_byte_in(0x21);
    port_byte_out(0x21, pic_mask | 0x01);

    // Re-register the handler at vector 32 to receive APIC signals instead of PIC
    register_interrupt_handler(32, timer_callback);

    // Configure LAPIC Timer: Periodic Mode (Bit 17) + Vector 32
    *LAPIC_R_TIMER_LVT = 32 | 0x20000;
    *LAPIC_R_TIMER_DIV = 0x3; // Divide-by-16
    *LAPIC_R_TIMER_INIT_CNT = ticks_per_period;

    is_apic_active = 1;
}

int apic_timer_active() {
    return is_apic_active;
}

unsigned int timer_get_ticks() {
    return timer_ticks;
}