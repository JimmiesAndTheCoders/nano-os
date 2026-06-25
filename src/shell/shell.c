#include "shell.h"
#include "screen.h"
#include "util.h"

char current_dir[64] = "/";

void print_prompt() {
    print("nano:");
    print(current_dir);
    print("> ");
}