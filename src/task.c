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

    // --- Ring 0 "Fake" Interrupt Stack ---
    *(--stack) = 0x0202;           // EFLAGS (Interrupts enabled)
    *(--stack) = 0x08;             // CS (Kernel Code Segment)
    *(--stack) = (unsigned int)entry; // EIP (Starting point)

    *(--stack) = 0;                // Error code
    *(--stack) = 32;               // Interrupt number

    // Pushed by pusha
    *(--stack) = 0; // EAX
    *(--stack) = 0; // ECX
    *(--stack) = 0; // EDX
    *(--stack) = 0; // EBX
    *(--stack) = 0; // ESP
    *(--stack) = 0; // EBP
    *(--stack) = 0; // ESI
    *(--stack) = 0; // EDI

    *(--stack) = 0x10; // DS (Data Segment)

    tasks[task_count].esp = (unsigned int)stack;
    tasks[task_count].active = 1;
    
    // Copy name
    int i = 0;
    while(name[i] && i < 15) { tasks[task_count].name[i] = name[i]; i++; }
    tasks[task_count].name[i] = '\0';
    
    task_count++;
}

// The Scheduler: Called by the timer
void schedule(registers_t *regs) {
    if (task_count <= 1) return;

    // Save current task's stack pointer
    tasks[current_task].esp = (unsigned int)regs;

    // Switch to next task
    current_task = (current_task + 1) % task_count;

    // Point the registers pointer to the NEW task's stack
    // The assembly 'mov esp, eax' will then physically jump to this stack
    *((unsigned int*)&regs) = tasks[current_task].esp;
}