#ifndef PANIC_H
#define PANIC_H

#include "cpu.h"

#ifdef __cplusplus
extern "C" {
#endif

// The central authority for system failure
void kpanic(const char* message, registers_t* regs);

// A diagnostic helper to walk the stack
void kpanic_backtrace(unsigned int max_frames);

#ifdef __cplusplus
}
#endif

#endif