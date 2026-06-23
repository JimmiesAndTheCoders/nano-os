#ifndef PORTS_H
#define PORTS_H

/* Reads a byte from the specified hardware port */
unsigned char port_byte_in(unsigned short port);

/* Writes a byte to the specified hardware port */
void port_byte_out(unsigned short port, unsigned char data);

/* Reads a word from the specified hardware port */
unsigned short port_word_in(unsigned short port);

/* Writes a word to the specified hardware port */
void port_word_out(unsigned short port, unsigned short data);

/* Reads a double word from the specified hardware port */
unsigned int port_dword_in(unsigned short port);

/* Writes a double word to the specified hardware port */
void port_dword_out(unsigned short port, unsigned int data);

#endif