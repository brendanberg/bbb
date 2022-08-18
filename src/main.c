#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <memory.h>

#include "cpu.h"

int main (int argc, char *argsv[]) {
    machine *m = machine_init(64 * 1024);

    machine_show(m);

    uint8_t prog[1024] = {
        MOV, REGISTER_CV, REGISTER_A, 4, //  3
        MOV, REGISTER_CV, REGISTER_B, 9, //  7
        ADD, REGISTER_A, REGISTER_B,     // 10
        DEC, REGISTER_B,                 // 12
        JMP, 1, 11, 0, 0, 0,             // 18
        MOV, REGISTER_MD, REGISTER_C, 0, 15, 15, 15,
        MOV, REGISTER_MD, REGISTER_D, 1, 15, 15, 15,
        MOV, REGISTER_MD, REGISTER_E, 2, 15, 15, 15,
        MOV, REGISTER_MD, REGISTER_F, 3, 15, 15, 15,
        CMP, REGISTER_CV, REGISTER_C, 0,
        JMP, 1, 13, 5, 0, 0,// 19
        CMP, REGISTER_CV, REGISTER_D, 0,
        JMP, 1, 13, 5, 0, 0,
        CMP, REGISTER_CV, REGISTER_E, 0,
        JMP, 1, 13, 5, 0, 0,
        CMP, REGISTER_CV, REGISTER_F, 0,
        JMP, 1, 13, 5, 0, 0,
        JMP, 9, 3, 1, 0, 0, // 92
        OR, REGISTER_CV, REGISTER_S1, 2,
    };
    memcpy(m->memory->data, prog, 1024);

    machine_run(m);
    machine_free(m);
    return 0;
}
