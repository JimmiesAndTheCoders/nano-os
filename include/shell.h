#ifndef SHELL_H
#define SHELL_H

void process_command(char *input);
void print_prompt();

/* Terminal Code Editor Hooks */
int shell_editor_active();
void shell_editor_handle_key(unsigned char scancode);
void cnode_launch(const char* filename, const char* cur_dir);

/* Path Resolution Utility Helpers */
void build_full_path(const char* current, const char* relative, char* dest);
void get_parent_directory(const char* path, char* dest);

#endif