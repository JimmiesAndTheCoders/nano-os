#include "elf.h"
#include "vfs.h"
#include "kmalloc.h"
#include "util.h"
#include "pmm.h"
#include "task.h"
#include "screen.h"

typedef struct elf_obj {
    char name[64];
    unsigned int base_addr;
    unsigned int entry;
    Elf32_Dyn* dynamic;
    Elf32_Sym* symtab;
    char* strtab;
    unsigned int* hash;
    Elf32_Rel* rel;
    unsigned int relsz;
    Elf32_Rel* jmprel;
    unsigned int pltrelsz;
    struct elf_obj* next;
} elf_obj_t;

static elf_obj_t* loaded_objects = 0;
static unsigned int next_lib_addr = 0x40000000;

void map_page(unsigned int* pd, unsigned int vaddr, unsigned int paddr, unsigned int flags) {
    unsigned int pde_idx = vaddr >> 22;
    unsigned int pte_idx = (vaddr >> 12) & 0x3FF;

    if ((pd[pde_idx] & 1) == 0) {
        unsigned int* pt = (unsigned int*)pmm_alloc_block();
        memset(pt, 0, PAGE_SIZE);
        pd[pde_idx] = (unsigned int)pt | flags;
    }

    unsigned int* pt = (unsigned int*)(pd[pde_idx] & 0xFFFFF000);
    pt[pte_idx] = (paddr & 0xFFFFF000) | flags;

    __asm__ __volatile__("invlpg (%0)" : : "r"(vaddr) : "memory");
}

static elf_obj_t* find_object(const char* name) {
    for (elf_obj_t* obj = loaded_objects; obj; obj = obj->next) {
        if (strcmp(obj->name, name) == 0) return obj;
    }
    return 0;
}

static int load_elf_object(const char* filepath, unsigned int* user_pd, unsigned int is_main, elf_obj_t** out_obj) {
    vfs_node_t* file = vfs_resolve_path(filepath);
    if (!file) {
        // Fallback search directory for libraries
        if (!is_main) {
            char fallback[128] = "/";
            memory_copy(filepath, fallback + 1, strlen(filepath) + 1);
            file = vfs_resolve_path(fallback);
        }
        if (!file) return -1;
    }

    Elf32_Ehdr ehdr;
    if (vfs_read(file, 0, sizeof(Elf32_Ehdr), (unsigned char*)&ehdr) != sizeof(Elf32_Ehdr)) {
        vfs_close(file); return -1;
    }

    if (ehdr.e_ident[0] != ELF_MAGIC_0 || ehdr.e_ident[4] != 1) { 
        vfs_close(file); return -2; 
    }

    unsigned int base_addr = is_main ? 0 : next_lib_addr;

    Elf32_Phdr* phdrs = (Elf32_Phdr*)kmalloc(ehdr.e_phnum * sizeof(Elf32_Phdr));
    vfs_read(file, ehdr.e_phoff, ehdr.e_phnum * sizeof(Elf32_Phdr), (unsigned char*)phdrs);

    Elf32_Dyn* dyn = 0;
    unsigned int max_vaddr = 0;

    for (unsigned int i = 0; i < ehdr.e_phnum; i++) {
        if (phdrs[i].p_type == PT_LOAD) {
            unsigned int vaddr = phdrs[i].p_vaddr + base_addr;
            unsigned int num_pages = (phdrs[i].p_memsz + (vaddr & 0xFFF) + 4095) / 4096;
            unsigned int vaddr_page = vaddr & 0xFFFFF000;

            for (unsigned int j = 0; j < num_pages; j++) {
                unsigned int cur = vaddr_page + j * 4096;
                unsigned int pde_idx = cur >> 22;
                unsigned int pte_idx = (cur >> 12) & 0x3FF;
                int mapped = 0;

                if (user_pd[pde_idx] & 1) {
                    unsigned int* pt = (unsigned int*)(user_pd[pde_idx] & 0xFFFFF000);
                    if (pt[pte_idx] & 1) mapped = 1;
                }

                if (!mapped) {
                    unsigned int paddr = (unsigned int)pmm_alloc_block();
                    memset((void*)paddr, 0, 4096);
                    map_page(user_pd, cur, paddr, 7); // Present + R/W + User Space
                }
            }

            // Since CR3 is swapped early, we can directly read into user memory addresses!
            if (phdrs[i].p_filesz > 0) {
                vfs_read(file, phdrs[i].p_offset, phdrs[i].p_filesz, (unsigned char*)vaddr);
            }
            
            // Zero the uninitialized bss chunk
            if (phdrs[i].p_memsz > phdrs[i].p_filesz) {
                memset((void*)(vaddr + phdrs[i].p_filesz), 0, phdrs[i].p_memsz - phdrs[i].p_filesz);
            }

            unsigned int end = phdrs[i].p_vaddr + phdrs[i].p_memsz;
            if (end > max_vaddr) max_vaddr = end;

        } else if (phdrs[i].p_type == PT_DYNAMIC) {
            dyn = (Elf32_Dyn*)(phdrs[i].p_vaddr + base_addr);
        }
    }

    if (!is_main) {
        next_lib_addr = (base_addr + max_vaddr + 0xFFF) & 0xFFFFF000;
    }

    elf_obj_t* obj = (elf_obj_t*)kmalloc(sizeof(elf_obj_t));
    memset(obj, 0, sizeof(elf_obj_t));
    obj->base_addr = base_addr;
    obj->entry = ehdr.e_entry + base_addr;
    obj->dynamic = dyn;
    
    const char* name = filepath;
    for(int k = strlen(filepath) - 1; k >= 0; k--) {
        if (filepath[k] == '/') { name = filepath + k + 1; break; }
    }
    memory_copy(name, obj->name, strlen(name) + 1);

    obj->next = loaded_objects;
    loaded_objects = obj;
    if (out_obj) *out_obj = obj;

    kfree(phdrs);
    vfs_close(file);
    return 0;
}

static void parse_dynamic(elf_obj_t* obj) {
    if (!obj->dynamic) return;
    Elf32_Dyn* d = obj->dynamic;
    while (d->d_tag != DT_NULL) {
        switch (d->d_tag) {
            case DT_STRTAB:   obj->strtab   = (char*)(obj->base_addr + d->d_un.d_ptr); break;
            case DT_SYMTAB:   obj->symtab   = (Elf32_Sym*)(obj->base_addr + d->d_un.d_ptr); break;
            case DT_HASH:     obj->hash     = (unsigned int*)(obj->base_addr + d->d_un.d_ptr); break;
            case DT_REL:      obj->rel      = (Elf32_Rel*)(obj->base_addr + d->d_un.d_ptr); break;
            case DT_RELSZ:    obj->relsz    = d->d_un.d_val; break;
            case DT_JMPREL:   obj->jmprel   = (Elf32_Rel*)(obj->base_addr + d->d_un.d_ptr); break;
            case DT_PLTRELSZ: obj->pltrelsz = d->d_un.d_val; break;
        }
        d++;
    }
}

static void load_dependencies(elf_obj_t* obj, unsigned int* user_pd) {
    if (!obj->dynamic) return;
    Elf32_Dyn* d = obj->dynamic;
    while (d->d_tag != DT_NULL) {
        if (d->d_tag == DT_NEEDED && obj->strtab) {
            const char* lib_name = obj->strtab + d->d_un.d_val;
            if (!find_object(lib_name)) {
                elf_obj_t* new_lib = 0;
                if (load_elf_object(lib_name, user_pd, 0, &new_lib) == 0) {
                    parse_dynamic(new_lib);
                    load_dependencies(new_lib, user_pd);
                } else {
                    print("Warning: Could not load required library: ");
                    print(lib_name); print("\n");
                }
            }
        }
        d++;
    }
}

static unsigned int resolve_symbol(const char* name) {
    for (elf_obj_t* obj = loaded_objects; obj; obj = obj->next) {
        if (obj->symtab && obj->strtab && obj->hash) {
            unsigned int nchain = obj->hash[1]; // Total symbols stored here via standard SYSV hash
            for (unsigned int i = 1; i < nchain; i++) {
                Elf32_Sym* sym = &obj->symtab[i];
                if (sym->st_shndx != 0) { // Symbol is fully defined
                    if (strcmp(obj->strtab + sym->st_name, name) == 0) {
                        return obj->base_addr + sym->st_value;
                    }
                }
            }
        }
    }
    return 0;
}

static void apply_relocs(elf_obj_t* obj, Elf32_Rel* rel_table, unsigned int size) {
    if (!rel_table) return;
    unsigned int num = size / sizeof(Elf32_Rel);
    for (unsigned int i = 0; i < num; i++) {
        Elf32_Rel* rel = &rel_table[i];
        unsigned int type = ELF32_R_TYPE(rel->r_info);
        unsigned int sym_idx = ELF32_R_SYM(rel->r_info);
        unsigned int* addr = (unsigned int*)(obj->base_addr + rel->r_offset);

        if (type == R_386_RELATIVE) {
            *addr += obj->base_addr;
        } else if (type == R_386_32 || type == R_386_GLOB_DAT || type == R_386_JMP_SLOT) {
            if (sym_idx > 0 && obj->symtab && obj->strtab) {
                const char* sym_name = obj->strtab + obj->symtab[sym_idx].st_name;
                unsigned int sym_val = resolve_symbol(sym_name);
                if (sym_val != 0) {
                    if (type == R_386_32) *addr += sym_val;
                    else *addr = sym_val;
                }
            }
        }
    }
}

int elf_load_and_run(const char* filepath, const char* task_name, int argc, char** argv, int envc, char** envp) {
    // 1. Allocate process-specific page directory mapping 
    unsigned int* user_pd = (unsigned int*)pmm_alloc_block();
    memset(user_pd, 0, 4096);
    
    // Copy the kernel space (First 4MB identity mapped mapping elements)
    for (int i = 0; i < 1024; i++) {
        user_pd[i] = ((unsigned int*)0x9000)[i];
    }

    // 2. Switch CR3 early - Kernel seamlessly accesses user mapped addresses!
    unsigned int old_cr3;
    __asm__ __volatile__("mov %%cr3, %0" : "=r"(old_cr3));
    __asm__ __volatile__("mov %0, %%cr3" : : "r"((unsigned int)user_pd));

    loaded_objects = 0;
    next_lib_addr = 0x40000000;

    // 3. Mount Main Executable
    elf_obj_t* main_obj = 0;
    int status = load_elf_object(filepath, user_pd, 1, &main_obj);
    if (status != 0) {
        __asm__ __volatile__("mov %0, %%cr3" : : "r"(old_cr3));
        return status;
    }

    // 4. Resolve Dynamic Segments and Required .so File Loaders
    parse_dynamic(main_obj);
    load_dependencies(main_obj, user_pd);

    // 5. Apply Relocations universally across process
    for (elf_obj_t* obj = loaded_objects; obj; obj = obj->next) {
        apply_relocs(obj, obj->rel, obj->relsz);
        apply_relocs(obj, obj->jmprel, obj->pltrelsz);
    }

    // 6. Map and Create the User Stack
    unsigned int user_stack_virtual = 0x0A000000;
    unsigned int user_stack_size = 16384; 
    for (unsigned int i = 0; i < user_stack_size / 4096; i++) {
        void* paddr = pmm_alloc_block();
        memset(paddr, 0, 4096);
        map_page(user_pd, user_stack_virtual + i * 4096, (unsigned int)paddr, 7);
    }
    
    // 7. Inject standard Linux C ABI parameter execution blocks (argc/argv/envp)
    char* str_ptr = (char*)(user_stack_virtual + user_stack_size);
    unsigned int argv_ptrs[32];
    unsigned int envp_ptrs[32];
    
    for (int i = envc - 1; i >= 0; i--) {
        str_ptr -= (strlen(envp[i]) + 1);
        memory_copy(envp[i], str_ptr, strlen(envp[i]) + 1);
        envp_ptrs[i] = (unsigned int)str_ptr;
    }
    for (int i = argc - 1; i >= 0; i--) {
        str_ptr -= (strlen(argv[i]) + 1);
        memory_copy(argv[i], str_ptr, strlen(argv[i]) + 1);
        argv_ptrs[i] = (unsigned int)str_ptr;
    }
    
    // Setup Env Array Block
    unsigned int envp_array_base = ((unsigned int)str_ptr & ~3) - (envc + 1) * 4;
    unsigned int* envp_array = (unsigned int*)envp_array_base;
    for (int i = 0; i < envc; i++) envp_array[i] = envp_ptrs[i];
    envp_array[envc] = 0; // Standard null terminator
    
    // Setup Argv Array Block
    unsigned int argv_array_base = envp_array_base - (argc + 1) * 4;
    unsigned int* argv_array = (unsigned int*)argv_array_base;
    for (int i = 0; i < argc; i++) argv_array[i] = argv_ptrs[i];
    argv_array[argc] = 0; // Standard null terminator
    
    // Write Standard C execution parameter pointers explicitly onto stack pointer top
    unsigned int* stack_ptr = (unsigned int*)argv_array_base;
    *(--stack_ptr) = envp_array_base;
    *(--stack_ptr) = argv_array_base;
    *(--stack_ptr) = argc;
    *(--stack_ptr) = 0; // Dummy standard C-style return execution pointer offset
    
    // Cleanup loaded library tracking mechanism allocations
    elf_obj_t* curr = loaded_objects;
    while(curr) {
        elf_obj_t* next = curr->next;
        kfree(curr);
        curr = next;
    }

    // 8. Restore the Kernel CR3 environment block
    __asm__ __volatile__("mov %0, %%cr3" : : "r"(old_cr3));

    // 9. Enqueue Task into Processor ring context pool
    extern void task_add_user_elf(void (*entry)(), unsigned int user_esp, unsigned int page_directory, const char *name);
    task_add_user_elf((void (*)())main_obj->entry, (unsigned int)stack_ptr, (unsigned int)user_pd, task_name);

    return 0;
}