#include "elf.h"
#include "vfs.h"
#include "kmalloc.h"
#include "util.h"
#include "pmm.h"
#include "task.h"
#include "screen.h"

static void map_page(unsigned int* pd, unsigned int vaddr, unsigned int paddr, unsigned int flags) {
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

static void user_memcpy(unsigned int* pd, unsigned int dest_vaddr, const void* src, unsigned int size) {
    unsigned char* src_ptr = (unsigned char*)src;
    unsigned int bytes_copied = 0;
    while (bytes_copied < size) {
        unsigned int cur_vaddr = dest_vaddr + bytes_copied;
        unsigned int pde_idx = cur_vaddr >> 22;
        unsigned int pte_idx = (cur_vaddr >> 12) & 0x3FF;
        
        unsigned int* pt = (unsigned int*)(pd[pde_idx] & 0xFFFFF000);
        unsigned int paddr = pt[pte_idx] & 0xFFFFF000;
        unsigned int offset = cur_vaddr & 0xFFF;
        
        unsigned int chunk = 4096 - offset;
        if (chunk > (size - bytes_copied)) chunk = size - bytes_copied;
        
        memory_copy((const char*)(src_ptr + bytes_copied), (char*)(paddr + offset), chunk);
        bytes_copied += chunk;
    }
}

int elf_load_and_run(const char* filepath, const char* task_name) {
    vfs_node_t* file = vfs_resolve_path(filepath);
    if (!file) {
        print("ELF Loader: File not found.\n");
        return -1;
    }

    if (file->flags & VFS_DIRECTORY) {
        print("ELF Loader: Path is a directory.\n");
        vfs_close(file);
        return -1;
    }

    Elf32_Ehdr ehdr;
    if (vfs_read(file, 0, sizeof(Elf32_Ehdr), (unsigned char*)&ehdr) != sizeof(Elf32_Ehdr)) {
        print("ELF Loader: Failed to read ELF header.\n");
        vfs_close(file);
        return -1;
    }

    if (ehdr.e_ident[0] != ELF_MAGIC_0 || ehdr.e_ident[1] != ELF_MAGIC_1 ||
        ehdr.e_ident[2] != ELF_MAGIC_2 || ehdr.e_ident[3] != ELF_MAGIC_3) {
        print("ELF Loader: Invalid magic signature.\n");
        vfs_close(file);
        return -2;
    }

    if (ehdr.e_ident[4] != 1) { 
        print("ELF Loader: Not a 32-bit binary.\n");
        vfs_close(file);
        return -2;
    }

    // Allocate process specific page directory page
    unsigned int* user_pd = (unsigned int*)pmm_alloc_block();
    memset(user_pd, 0, 4096);

    // Share memory mapped space (First 64MB kernel structures)
    for (int i = 0; i < 1024; i++) {
        user_pd[i] = ((unsigned int*)0x9000)[i];
    }

    Elf32_Phdr* phdrs = (Elf32_Phdr*)kmalloc(ehdr.e_phnum * sizeof(Elf32_Phdr));
    if (vfs_read(file, ehdr.e_phoff, ehdr.e_phnum * sizeof(Elf32_Phdr), (unsigned char*)phdrs) != ehdr.e_phnum * sizeof(Elf32_Phdr)) {
        print("ELF Loader: Failed to read Program Headers.\n");
        kfree(phdrs);
        vfs_close(file);
        return -1;
    }

    for (unsigned int i = 0; i < ehdr.e_phnum; i++) {
        if (phdrs[i].p_type == PT_LOAD) {
            unsigned int vaddr_page = phdrs[i].p_vaddr & 0xFFFFF000;
            unsigned int vaddr_offset = phdrs[i].p_vaddr & 0xFFF;
            unsigned int num_pages = (phdrs[i].p_memsz + vaddr_offset + 4095) / 4096;

            for (unsigned int j = 0; j < num_pages; j++) {
                unsigned int cur_vaddr = vaddr_page + j * 4096;
                unsigned int pde_idx = cur_vaddr >> 22;
                unsigned int pte_idx = (cur_vaddr >> 12) & 0x3FF;
                unsigned int paddr = 0;

                if (user_pd[pde_idx] & 1) {
                    unsigned int* pt = (unsigned int*)(user_pd[pde_idx] & 0xFFFFF000);
                    if (pt[pte_idx] & 1) {
                        paddr = pt[pte_idx] & 0xFFFFF000;
                    }
                }

                if (paddr == 0) {
                    paddr = (unsigned int)pmm_alloc_block();
                    memset((void*)paddr, 0, 4096);
                    map_page(user_pd, cur_vaddr, paddr, 7); // Present + R/W + User Space
                }
            }

            if (phdrs[i].p_filesz > 0) {
                unsigned char* temp_buf = (unsigned char*)kmalloc(phdrs[i].p_filesz);
                vfs_read(file, phdrs[i].p_offset, phdrs[i].p_filesz, temp_buf);
                user_memcpy(user_pd, phdrs[i].p_vaddr, temp_buf, phdrs[i].p_filesz);
                kfree(temp_buf);
            }
        }
    }

    kfree(phdrs);
    vfs_close(file);

    // Setup User Stack (8KB mapped at 0x0A000000)
    unsigned int user_stack_virtual = 0x0A000000;
    unsigned int user_stack_size = 8192;
    for (unsigned int i = 0; i < user_stack_size / 4096; i++) {
        void* paddr = pmm_alloc_block();
        memset(paddr, 0, 4096);
        map_page(user_pd, user_stack_virtual + i * 4096, (unsigned int)paddr, 7);
    }
    unsigned int user_esp = user_stack_virtual + user_stack_size;

    // Enqueue program entry in tasking scheduler
    extern void task_add_user_elf(void (*entry)(), unsigned int user_esp, unsigned int page_directory, const char *name);
    task_add_user_elf((void (*)())ehdr.e_entry, user_esp, (unsigned int)user_pd, task_name);

    return 0;
}