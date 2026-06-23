#ifndef SHELL_H
#define SHELL_H

void process_command(char *input);
void print_prompt();

/* Terminal Code Editor Hooks */
int shell_editor_active();
void shell_editor_handle_key(unsigned char scancode);

#endif