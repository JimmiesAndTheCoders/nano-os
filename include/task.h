#ifndef TASK_H
#define TASK_H

#include "cpu.h"

void init_tasking();
void task_add(void (*entry)(), const char *name);
void task_add_user(void (*entry)(), const char *name); 
void task_add_user_elf(void (*entry)(), unsigned int user_esp, unsigned int page_directory, const char *name);
unsigned int schedule(registers_t *regs);

#endif