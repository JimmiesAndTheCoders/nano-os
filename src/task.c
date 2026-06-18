#include "task.h"
#include "screen.h"

#define MAX_TASKS 4

typedef struct task {
    unsigned int esp;
    unsigned int ebp;
    unsigned int eip;
    unsigned int active;
    char name[16];
} task_t;

static task_t tasks[MAX_TASKS];
static unsigned int current_task = 0;
static unsigned int task_count = 0;

void task_init_task_context(int index, unsigned int entry_point, unsigned int stack_top, const char *name) {
    tasks[index].esp = stack_top;
    tasks[index].ebp = stack_top;
    tasks[index].eip = entry_point;
    tasks[index].active = 1;

    int i = 0;
    while (name[i] != '\0' && i < 15) {
        tasks[index].name[i] = name[i];
        i++;
    }
    tasks[index].name[i] = '\0';
}

void init_tasking() {
    int i;
    for (i = 0; i < MAX_TASKS; i++) {
        tasks[i].active = 0;
    }

    task_init_task_context(0, 0, 0, "main");
    task_count = 1;
    current_task = 0;
}

void task_add(void (*entry)(), unsigned int stack_top, const char *name) {
    if (task_count >= MAX_TASKS) {
        return;
    }

    task_init_task_context(task_count, (unsigned int)entry, stack_top, name);
    task_count++;
}

void schedule(registers_t *regs) {
    if (task_count <= 1) {
        return;
    }

    unsigned int next = (current_task + 1) % task_count;
    unsigned int start = next;

    while (!tasks[next].active) {
        next = (next + 1) % task_count;
        if (next == start) {
            return;
        }
    }

    if (next == current_task) {
        return;
    }

    tasks[current_task].esp = regs->esp;
    tasks[current_task].ebp = regs->ebp;
    tasks[current_task].eip = regs->eip;

    regs->esp = tasks[next].esp;
    regs->ebp = tasks[next].ebp;
    regs->eip = tasks[next].eip;

    current_task = next;
}
