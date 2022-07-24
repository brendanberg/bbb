#ifndef BBB_CPU_H
#define BBB_CPU_H

#include <stdint.h>
#include "memory.h"


enum machine_state {
    RUN,
    HALT
};

typedef struct machine {
    uint16_t registers [16];
    memory *memory;
} machine;

void machine_init (machine *m, size_t size);
int machine_load ();
int store ();

void machine_instr_fetch (machine *m);
void machine_instr_decode (machine *m);
void machine_instr_execute (machine *m);

void machine_free (machine *m);

#endif
