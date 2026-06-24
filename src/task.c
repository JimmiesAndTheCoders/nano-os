#include "task.h"
#include "kmalloc.h"
#include "util.h"
#include "gdt.h" 

#define MAX_TASKS 8
#define STACK_SIZE 4096

typedef struct task {
    unsigned int esp;      
    unsigned int kernel_stack; 
    unsigned int page_directory; // Process isolation page table base (CR3)
    unsigned int active;   
    char name[16];
} task_t;

static task_t tasks[MAX_TASKS];
static int current_task = -1;
static int task_count = 0;

void init_tasking() {
    for (int i = 0; i < MAX_TASKS; i++) {
        tasks[i].active = 0;
        tasks[i].page_directory = 0;
    }

    tasks[0].active = 1;
    tasks[0].kernel_stack = 0x90000; 
    tasks[0].page_directory = 0x9000; // Default boot page directory
    memory_copy("kernel", tasks[0].name, 7);
    current_task = 0;
    task_count = 1;
}

void task_add(void (*entry)(), const char *name) {
    if (task_count >= MAX_TASKS) return;

    unsigned int stack_mem = (unsigned int)kmalloc(STACK_SIZE);
    unsigned int *stack = (unsigned int *)(stack_mem + STACK_SIZE);

    *(--stack) = 0x0202;           
    *(--stack) = 0x08;             
    *(--stack) = (unsigned int)entry; 

    *(--stack) = 0;                
    *(--stack) = 32;               

    *(--stack) = 0; *(--stack) = 0; *(--stack) = 0; *(--stack) = 0; 
    *(--stack) = 0; *(--stack) = 0; *(--stack) = 0; *(--stack) = 0; 

    *(--stack) = 0x10; 

    tasks[task_count].esp = (unsigned int)stack;
    tasks[task_count].kernel_stack = stack_mem + STACK_SIZE; 
    tasks[task_count].page_directory = 0x9000; 
    tasks[task_count].active = 1;
    
    int i = 0;
    while(name[i] && i < 15) { tasks[task_count].name[i] = name[i]; i++; }
    tasks[task_count].name[i] = '\0';
    
    task_count++;
}

void task_add_user(void (*entry)(), const char *name) {
    if (task_count >= MAX_TASKS) return;

    unsigned int kernel_stack_mem = (unsigned int)kmalloc(STACK_SIZE);
    unsigned int user_stack_mem = (unsigned int)kmalloc(STACK_SIZE);

    unsigned int *stack = (unsigned int *)(kernel_stack_mem + STACK_SIZE);

    *(--stack) = 0x23;                        
    *(--stack) = user_stack_mem + STACK_SIZE; 
    *(--stack) = 0x0202;                      
    *(--stack) = 0x1B;                        
    *(--stack) = (unsigned int)entry;         

    *(--stack) = 0;                
    *(--stack) = 32;               

    *(--stack) = 0; *(--stack) = 0; *(--stack) = 0; *(--stack) = 0; 
    *(--stack) = 0; *(--stack) = 0; *(--stack) = 0; *(--stack) = 0; 

    *(--stack) = 0x23;                        

    tasks[task_count].esp = (unsigned int)stack;
    tasks[task_count].kernel_stack = kernel_stack_mem + STACK_SIZE;
    tasks[task_count].page_directory = 0x9000;
    tasks[task_count].active = 1;
    
    int i = 0;
    while(name[i] && i < 15) { tasks[task_count].name[i] = name[i]; i++; }
    tasks[task_count].name[i] = '\0';
    
    task_count++;
}

void task_add_user_elf(void (*entry)(), unsigned int user_esp, unsigned int page_directory, const char *name) {
    if (task_count >= MAX_TASKS) return;

    unsigned int kernel_stack_mem = (unsigned int)kmalloc(STACK_SIZE);
    unsigned int *stack = (unsigned int *)(kernel_stack_mem + STACK_SIZE);

    *(--stack) = 0x23;                        // SS (User Mode Data Segment)
    *(--stack) = user_esp;                    // ESP (User Mode Stack pointer)
    *(--stack) = 0x0202;                      // EFLAGS (Interrupts enabled)
    *(--stack) = 0x1B;                        // CS (User Mode Code Segment)
    *(--stack) = (unsigned int)entry;         // EIP

    *(--stack) = 0;                
    *(--stack) = 32;               

    *(--stack) = 0; *(--stack) = 0; *(--stack) = 0; *(--stack) = 0; 
    *(--stack) = 0; *(--stack) = 0; *(--stack) = 0; *(--stack) = 0; 

    *(--stack) = 0x23;                        // DS (User Mode Data Segment)

    tasks[task_count].esp = (unsigned int)stack;
    tasks[task_count].kernel_stack = kernel_stack_mem + STACK_SIZE;
    tasks[task_count].page_directory = page_directory;
    tasks[task_count].active = 1;
    
    int i = 0;
    while(name[i] && i < 15) { tasks[task_count].name[i] = name[i]; i++; }
    tasks[task_count].name[i] = '\0';
    
    task_count++;
}

unsigned int schedule(registers_t *regs) {
    if (task_count <= 1) return (unsigned int)regs;

    tasks[current_task].esp = (unsigned int)regs;

    current_task++;
    if (current_task >= task_count) current_task = 0;

    tss_set_stack(tasks[current_task].kernel_stack);

    // Switch Page Directory (Paging Isolation)
    unsigned int pd = tasks[current_task].page_directory;
    if (pd != 0) {
        __asm__ __volatile__("mov %0, %%cr3" : : "r"(pd));
    } else {
        __asm__ __volatile__("mov %0, %%cr3" : : "r"(0x9000));
    }

    return tasks[current_task].esp;
}