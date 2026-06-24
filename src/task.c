#include "task.h"
#include "kmalloc.h"
#include "util.h"
#include "gdt.h" 
#include "screen.h"
#include "signal.h"

#define MAX_TASKS 8
#define STACK_SIZE 4096

typedef struct task {
    unsigned int esp;      
    unsigned int kernel_stack; 
    unsigned int page_directory; // Process isolation page table base (CR3)
    unsigned int active;   
    char name[16];
    int pid;
    unsigned int pending_signals;
    int is_user;
} task_t;

static task_t tasks[MAX_TASKS];
static int current_task = -1;
static int task_count = 0;

void init_tasking() {
    for (int i = 0; i < MAX_TASKS; i++) {
        tasks[i].active = 0;
        tasks[i].page_directory = 0;
        tasks[i].pid = i;
        tasks[i].pending_signals = 0;
        tasks[i].is_user = 0;
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
    tasks[task_count].pid = task_count;
    tasks[task_count].pending_signals = 0;
    tasks[task_count].is_user = 1;
    
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
    tasks[task_count].pid = task_count;
    tasks[task_count].pending_signals = 0;
    tasks[task_count].is_user = 1;
    
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
    tasks[task_count].pid = task_count;
    tasks[task_count].pending_signals = 0;
    tasks[task_count].is_user = 1;
    
    int i = 0;
    while(name[i] && i < 15) { tasks[task_count].name[i] = name[i]; i++; }
    tasks[task_count].name[i] = '\0';
    
    task_count++;
}

int task_get_current() {
    return current_task;
}

void task_terminate(int pid) {
    if (pid <= 0 || pid >= task_count) return; // Protect kernel
    tasks[pid].active = 0;
}

void task_signal(int pid, int sig) {
    if (pid <= 0 || pid >= task_count) return; // Protect kernel
    tasks[pid].pending_signals |= (1 << sig);
}

void task_kill_foreground() {
    for (int i = task_count - 1; i > 0; i--) {
        if (tasks[i].active && tasks[i].is_user) {
            task_signal(i, SIGINT);
            return;
        }
    }
}

void task_list() {
    print("PID  STATE     TYPE      NAME\n");
    print("----------------------------------------------------------------\n");
    for (int i = 0; i < task_count; i++) {
        char buf[16];
        itoa(tasks[i].pid, buf);
        print(buf);
        if (tasks[i].pid < 10) print("    "); else print("   ");
        
        if (tasks[i].active) print("RUNNING   ");
        else print("ZOMBIE    ");

        if (tasks[i].is_user) print("USER      ");
        else print("KERNEL    ");

        print(tasks[i].name);
        print("\n");
    }
}

unsigned int schedule(registers_t *regs) {
    if (task_count <= 1) return (unsigned int)regs;

    tasks[current_task].esp = (unsigned int)regs;

    int starting_task = current_task;
    do {
        current_task++;
        if (current_task >= task_count) current_task = 0;
        
        if (tasks[current_task].active) {
            // Check for default action process termination signals
            if ((tasks[current_task].pending_signals & (1 << SIGKILL)) ||
                (tasks[current_task].pending_signals & (1 << SIGINT))  ||
                (tasks[current_task].pending_signals & (1 << SIGTERM))) 
            {
                tasks[current_task].active = 0;
            }
        }
        if (tasks[current_task].active) break;
    } while (current_task != starting_task);

    // Fallback to idle kernel task if no other active processes exist
    if (!tasks[current_task].active) {
        current_task = 0;
    }

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