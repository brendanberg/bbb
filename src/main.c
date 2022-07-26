#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <memory.h>

#include "cpu.h"

int main (int argc, char *argsv[]) {
    machine *m;
    machine_init(&m, 64 * 1024);

    machine_show(m);

    uint8_t prog[1024] = {
        MOV, REGISTER_CV, REGISTER_A, 4,
        MOV, REGISTER_CV, REGISTER_B, 9,
        ADD, REGISTER_A, REGISTER_B,
        DEC, REGISTER_B,
        JMP, 1, 11, 0, 0, 0,
        OR, REGISTER_CV, REGISTER_S1, 2,
    };
    memcpy(m->memory->data, prog, 1024);

    machine_run(m);
    machine_free(m);
    return 0;
}
