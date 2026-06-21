#include <stddef.h>

extern "C" {
    #include "kmalloc.h"

    void __cxa_pure_virtual() { while (1); }

    typedef void (*constructor)();
    extern constructor _start_ctors;
    extern constructor _end_ctors;

    void call_global_constructors() {
        for (constructor* i = &_start_ctors; i != &_end_ctors; ++i) {
            if (*i) (*i)();
        }
    }
}

void* operator new(size_t size) { 
    return kmalloc(size); 
}

void* operator new[](size_t size) { 
    return kmalloc(size); 
}

void operator delete(void* p) noexcept { 
    kfree(p); 
}

void operator delete[](void* p) noexcept { 
    kfree(p); 
}

void operator delete(void* p, size_t size) noexcept { 
    (void)size;
    kfree(p); 
}

void operator delete[](void* p, size_t size) noexcept { 
    (void)size;
    kfree(p); 
}