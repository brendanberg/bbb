#ifndef BBB_ASSEM_H
#define BBB_ASSEM_H

#include "../machine/memory.h"

memory *assemble(char *prog);
memory *build_image(char *filename, char *prog);

#endif
