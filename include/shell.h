#ifndef SHELL_H
#define SHELL_H

/* Evaluates a user command entered into the CLI */
void process_command(char *input);

/* Displays the active command line prompt */
void print_prompt();

#endif