#ifndef PORTS_H
#define PORTS_H

/* Reads a byte from the specified hardware port */
unsigned char port_byte_in(unsigned short port);

/* Writes a byte to the specified hardware port */
void port_byte_out(unsigned short port, unsigned char data);

#endif
