#include "ports.h"

/* * Uses GCC inline assembly to interface with the CPU's 'in' instruction.
* '=a' (result) means: put AL register in variable 'result'.
* 'd' (port) means: load EDX register with variable 'port'.
*/
unsigned char port_byte_in(unsigned short port) {
	unsigned char result;
	__asm__("in %%dx, %%al" : "=a" (result) : "d" (port));
   	return result;
}

/* * Uses GCC inline assembly to interface with the CPU's 'out' instruction.
* 'a' (data) means: load EAX register with variable 'data'.
* 'd' (port) means: load EDX register with variable 'port'.
*/
void port_byte_out(unsigned short port, unsigned char data) {
	__asm__("out %%al, %%dx" : : "a" (data), "d" (port));
}
