#ifndef BBB_IO_H
#define BBB_IO_H

#include <stdint.h>

void kbio_setup();
uint16_t kbio_get_keymap(char *);

#endif
