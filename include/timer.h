#ifndef TIMER_H
#define TIMER_H

void init_timer(unsigned int frequency);
unsigned int timer_get_ticks();

/* Local APIC Timer Interfaces */
void init_apic_timer(unsigned int frequency);
int apic_timer_active();

#endif
