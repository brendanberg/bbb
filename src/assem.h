#ifndef BBB_ASSEM_H
#define BBB_ASSEM_H

#include "memory.h"

memory *assemble(char *prog);
memory *build_image(char *filename, char *prog, bool verbose);

#endif
