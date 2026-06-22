#ifndef GDT_H
#define GDT_H

void init_gdt();
void tss_set_stack(unsigned int kernel_esp);

#endif