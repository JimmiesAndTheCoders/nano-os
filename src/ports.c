#include "ports.h"

/* Uses GCC inline assembly to interface with the CPU's 'in' instruction. */
unsigned char port_byte_in(unsigned short port) {
    unsigned char result;
    __asm__("in %%dx, %%al" : "=a" (result) : "d" (port));
    return result;
}

/* Uses GCC inline assembly to interface with the CPU's 'out' instruction. */
void port_byte_out(unsigned short port, unsigned char data) {
    __asm__("out %%al, %%dx" : : "a" (data), "d" (port));
}

/* Reads a word from the specified hardware port */
unsigned short port_word_in(unsigned short port) {
    unsigned short result;
    __asm__("in %%dx, %%ax" : "=a" (result) : "d" (port));
    return result;
}

/* Writes a word to the specified hardware port */
void port_word_out(unsigned short port, unsigned short data) {
    __asm__("out %%ax, %%dx" : : "a" (data), "d" (port));
}

/* Reads a double word from the specified hardware port */
unsigned int port_dword_in(unsigned short port) {
    unsigned int result;
    __asm__("inl %%dx, %%eax" : "=a" (result) : "d" (port));
    return result;
}

/* Writes a double word to the specified hardware port */
void port_dword_out(unsigned short port, unsigned int data) {
    __asm__("outl %%eax, %%dx" : : "a" (data), "d" (port));
}