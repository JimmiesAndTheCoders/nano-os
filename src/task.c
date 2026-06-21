#include "task.h"
#include "kmalloc.h"
#include "util.h"

#define MAX_TASKS 8
#define STACK_SIZE 4096

typedef struct task {
    unsigned int esp;      // Current stack pointer
    unsigned int active;   // Is task running?
    char name[16];
} task_t;

static task_t tasks[MAX_TASKS];
static int current_task = -1;
static int task_count = 0;

void init_tasking() {
    for (int i = 0; i < MAX_TASKS; i++) tasks[i].active = 0;

    // Task 0 is the "Kernel/Shell" task (already running)
    tasks[0].active = 1;
    memory_copy("kernel", tasks[0].name, 7);
    current_task = 0;
    task_count = 1;
}

void task_add(void (*entry)(), const char *name) {
    if (task_count >= MAX_TASKS) return;

    unsigned int stack_mem = (unsigned int)kmalloc(STACK_SIZE);
    unsigned int *stack = (unsigned int *)(stack_mem + STACK_SIZE);

    // 1. The IRET frame
    *(--stack) = 0x0202;           // EFLAGS (Interrupts enabled)
    *(--stack) = 0x08;             // CS
    *(--stack) = (unsigned int)entry; // EIP

    // 2. The Error code and Int No (expected by isr_common_stub)
    *(--stack) = 0;                // Error code
    *(--stack) = 32;               // Int no

    // 3. The PUSHA frame (Order: EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI)
    *(--stack) = 0; // EAX
    *(--stack) = 0; // ECX
    *(--stack) = 0; // EDX
    *(--stack) = 0; // EBX
    *(--stack) = 0; // ESP
    *(--stack) = 0; // EBP
    *(--stack) = 0; // ESI
    *(--stack) = 0; // EDI

    // 4. The Data Segment
    *(--stack) = 0x10; // DS

    tasks[task_count].esp = (unsigned int)stack;
    tasks[task_count].active = 1;
    
    // Copy name
    int i = 0;
    while(name[i] && i < 15) { tasks[task_count].name[i] = name[i]; i++; }
    tasks[task_count].name[i] = '\0';
    
    task_count++;
}

// The Scheduler: Called by the timer
unsigned int schedule(registers_t *regs) {
    // If we only have the kernel task, don't switch anything.
    if (task_count <= 1) {
        return (unsigned int)regs;
    }

    // Save the stack pointer of the task that just got interrupted
    tasks[current_task].esp = (unsigned int)regs;

    // Pick the next task
    current_task++;
    if (current_task >= task_count) {
        current_task = 0;
    }

    // Return the stack pointer of the NEW task
    return tasks[current_task].esp;
}