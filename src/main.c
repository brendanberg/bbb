#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "cpu.h"

int main (int argc, char *argsv[]) {
    machine *m = malloc(1);
    machine_init(m, 64 * 1024);


    machine_free(m);
    return 0;
}
