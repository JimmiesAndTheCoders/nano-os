#ifndef TASK_H
#define TASK_H

#include "cpu.h"

void init_tasking();
void task_add(void (*entry)(), unsigned int stack_top, const char *name);
void schedule(registers_t *regs);

#endif
