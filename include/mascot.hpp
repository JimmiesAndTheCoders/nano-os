#ifndef MASCOT_HPP
#define MASCOT_HPP

extern "C" {
    #include "screen.h"
}

class Mascot {
public:
    Mascot(char icon) : icon(icon) {}
    void breathe();
private:
    char icon;
    int state = 0;
};

#endif