#ifndef TASK_H
#define TASK_H

#include "cpu.h"

void init_tasking();
void task_add(void (*entry)(), const char *name);
void task_add_user(void (*entry)(), const char *name); 
void task_add_user_elf(void (*entry)(), unsigned int user_esp, unsigned int page_directory, const char *name);
unsigned int schedule(registers_t *regs);

int task_get_current();
void task_signal(int pid, int sig);
void task_terminate(int pid);
void task_list();
void task_kill_foreground();
unsigned int task_get_heap_end(int pid);
void task_set_heap_end(int pid, unsigned int heap_end);
unsigned int task_get_page_directory(int pid);

#endif