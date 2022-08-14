#include <termios.h>
#include <curses.h>
#include <unistd.h>
#include <stdlib.h>

#include "io.h"


struct termios term_orig;

void kbio_teardown() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &term_orig);
}
void kbio_setup() {
    tcgetattr(STDIN_FILENO, &term_orig);
    atexit(kbio_teardown);

    struct termios term = term_orig;
    term.c_lflag &= ~(ECHO | ICANON);
    term.c_cc[VMIN] = 0;
    term.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &term);
}

uint16_t kbio_get_keymap() {
    uint16_t map = 0;
    char c;
    
    while (read(STDIN_FILENO, &c, 1) == 1) {
        switch (c) {
        case '0':
            map |= 0x0001;
            break;
        case '1':
            map |= 0x0002;
            break;
        case '2':
            map |= 0x0004;
            break;
        case '3':
            map |= 0x0008;
            break;
        case '4':
            map |= 0x0010;
            break;
        case '5':
            map |= 0x0020;
            break;
        case '6':
            map |= 0x0040;
            break;
        case '7':
            map |= 0x0080;
            break;
        case '8':
            map |= 0x0100;
            break;
        case '9':
            map |= 0x0200;
            break;
        case 'a':
        case 'A':
            map |= 0x0400;
            break;
        case 'b':
        case 'B':
            map |= 0x0800;
            break;
        case 'c':
        case 'C':
            map |= 0x1000;
            break;
        case 'd':
        case 'D':
            map |= 0x2000;
            break;
        case 'e':
        case 'E':
            map |= 0x4000;
            break;
        case 'f':
        case 'F':
            map |= 0x8000;
            break;        
        }
    }

    return map;
}
