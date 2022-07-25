#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "cpu.h"

int main (int argc, char *argsv[]) {
    machine *m;
    machine_init(&m, 64 * 1024);

    printf("C:         %d\n", m->registers[REGISTER_C]);
    printf("memory[7]: %d\n", m->memory->data[7]);
    m->memory->data[0] = ADD;
    m->memory->data[1] = REGISTER_MD;
    m->memory->data[2] = REGISTER_C;
    m->memory->data[3] = 7;
    m->memory->data[4] = 0;
    m->memory->data[5] = 0;
    m->memory->data[6] = 0;
    m->memory->data[7] = 4;
    machine_run(m);
    printf("C:         %d\n", m->registers[REGISTER_C]);
    printf("memory[7]: %d\n", m->memory->data[7]);
    machine_free(m);
    return 0;
}
