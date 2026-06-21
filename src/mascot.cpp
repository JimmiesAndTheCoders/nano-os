#include "mascot.hpp"

extern "C" void print(const char* message);

void Mascot::breathe() {
    if (state == 0) {
        print(" (");
        char str[2] = {icon, '\0'};
        print(str);
        print(") ");
        state = 1;
    } else {
        print("  ");
        state = 0;
    }
}