#ifndef BBB_TEST_CPU_H
#define BBB_TEST_CPU_H

#include "../machine/cpu.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wundefined-inline"
extern inline void machine_instr_fetch(machine *m);
extern inline void machine_instr_decode(machine *m);
extern inline void machine_instr_execute(machine *m);
extern inline void machine_interrupt_check(machine *m);
#pragma GCC diagnostic pop

#endif
