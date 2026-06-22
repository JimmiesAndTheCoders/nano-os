#include "gdt.h"

struct gdt_entry_struct {
    unsigned short limit_low;
    unsigned short base_low;
    unsigned char  base_middle;
    unsigned char  access;
    unsigned char  granularity;
    unsigned char  base_high;
} __attribute__((packed));
typedef struct gdt_entry_struct gdt_entry_t;

struct gdt_ptr_struct {
    unsigned short limit;
    unsigned int   base;
} __attribute__((packed));
typedef struct gdt_ptr_struct gdt_ptr_t;

struct tss_entry_struct {
    unsigned int prev_tss;
    unsigned int esp0, ss0, esp1, ss1, esp2, ss2;
    unsigned int cr3, eip, eflags, eax, ecx, edx, ebx, esp, ebp, esi, edi;
    unsigned int es, cs, ss, ds, fs, gs;
    unsigned int ldt;
    unsigned short trap, iomap_base;
} __attribute__((packed));
typedef struct tss_entry_struct tss_entry_t;

gdt_entry_t gdt_entries[6];
gdt_ptr_t   gdt_ptr;
tss_entry_t tss_entry;

extern void gdt_flush(unsigned int);
extern void tss_flush();

static void gdt_set_gate(int num, unsigned int base, unsigned int limit, unsigned char access, unsigned char gran) {
    gdt_entries[num].base_low    = (base & 0xFFFF);
    gdt_entries[num].base_middle = (base >> 16) & 0xFF;
    gdt_entries[num].base_high   = (base >> 24) & 0xFF;
    gdt_entries[num].limit_low   = (limit & 0xFFFF);
    gdt_entries[num].granularity = (limit >> 16) & 0x0F;
    gdt_entries[num].granularity |= gran & 0xF0;
    gdt_entries[num].access      = access;
}

static void write_tss(int num, unsigned short ss0, unsigned int esp0) {
    unsigned int base = (unsigned int) &tss_entry;
    unsigned int limit = sizeof(tss_entry);

    gdt_set_gate(num, base, limit, 0xE9, 0x00);

    // Zero out the TSS memory space
    char* tss_ptr = (char*)&tss_entry;
    for(unsigned int i = 0; i < sizeof(tss_entry); i++) tss_ptr[i] = 0;

    tss_entry.ss0  = ss0;
    tss_entry.esp0 = esp0;
    
    // Set Ring 3 Data & Code Selectors
    tss_entry.cs   = 0x1B; 
    tss_entry.ss = tss_entry.ds = tss_entry.es = tss_entry.fs = tss_entry.gs = 0x23; 
    tss_entry.iomap_base = sizeof(tss_entry);
}

void init_gdt() {
    gdt_ptr.limit = (sizeof(gdt_entry_t) * 6) - 1;
    gdt_ptr.base  = (unsigned int)&gdt_entries;

    gdt_set_gate(0, 0, 0, 0, 0);                // Null segment
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF); // Kernel Code segment
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF); // Kernel Data segment
    gdt_set_gate(3, 0, 0xFFFFFFFF, 0xFA, 0xCF); // User Code segment (Ring 3)
    gdt_set_gate(4, 0, 0xFFFFFFFF, 0xF2, 0xCF); // User Data segment (Ring 3)
    write_tss(5, 0x10, 0x0);                    // Task State Segment

    gdt_flush((unsigned int)&gdt_ptr);
    tss_flush();
}

void tss_set_stack(unsigned int kernel_esp) {
    tss_entry.esp0 = kernel_esp;
}