#ifndef TASK_H
#define TASK_H

#include "cpu.h"

void init_tasking();
void task_add(void (*entry)(), const char *name);
void task_add_user(void (*entry)(), const char *name); // <-- NEW
unsigned int schedule(registers_t *regs);

#endif