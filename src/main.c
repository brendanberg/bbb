#include <memory.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "cpu.h"
#define ADDR(w, x, y, z) (z), (y), (x), (w)

int main(int argc, char *argsv[]) {
    machine *m = machine_init(64 * 1024);

    machine_show(m);

    uint8_t prog[1024] = {
        MOV, REGISTER_CV, REGISTER_A, 4,
        MOV, REGISTER_CV, REGISTER_B, 9,  //  7
        ADD, REGISTER_A, REGISTER_B,      // 10
        DEC, REGISTER_B,                  // 12
        JMP, NZER, ADDR(0, 0, 0, 11),     // 18
        MOV, REGISTER_MD, REGISTER_C, ADDR(15, 15, 15, 0),
        MOV, REGISTER_MD, REGISTER_D, ADDR(15, 15, 15, 1),
        MOV, REGISTER_MD, REGISTER_E, ADDR(15, 15, 15, 2),
        MOV, REGISTER_MD, REGISTER_F, ADDR(15, 15, 15, 3),
        CMP, REGISTER_CV, REGISTER_C, 0,  // 50
        JMP, NZER, ADDR(0, 0, 5, 13),     // 56
        CMP, REGISTER_CV, REGISTER_D, 0,
        JMP, NZER, ADDR(0, 0, 5, 13),
        CMP, REGISTER_CV, REGISTER_E, 0,
        JMP, NZER, ADDR(0, 0, 5, 13),
        CMP, REGISTER_CV, REGISTER_F, 0,
        JMP, NZER, ADDR(0, 0, 5, 13),
        JMP, ZER, ADDR(0, 0, 1, 3),       // 92
        OR, REGISTER_CV, REGISTER_S1, 2,
    };
    memcpy(m->memory->data, prog, 1024);

    machine_run(m);
    machine_free(m);
    return 0;
}
