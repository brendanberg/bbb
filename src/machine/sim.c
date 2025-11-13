#include "cpu.h"
#include "io.h"
#include "memory.h"
// #include <stdbool.h>
// #include <stdint.h>
#include <stdio.h>
// #include <stdlib.h>

uint16_t prev_keymap = 0;

#define RENDER(t, c, e) ((t) ? printf((c), (t)) : printf((e), (t)))
#define E(x) "\e[" #x
#define E385(x) "\e[38:5:" #x
#define E0(x) "\e[0;" #x

void sim_setup(machine *m) {
    // fprintf(stderr, "Starting up...\n");
    printf("\n\n\n\n\n\n\n\n\n\n\n");
    kbio_setup();
}

void sim_print(machine *m) {
    // Return to home
    printf(E(11A) E(?25l) "\n" E385(13m));
    printf("╔═════════════╤════════╤══════════════════════════════╗\n");
    printf("║" E385(6m) " A B C D E F " E385(13m));
    printf("│" E385(6m) " HIOCZN " E385(13m));
    printf("│" E385(6m) " PROG  STAK  INTR  INDX  TEMP " E385(13m) "║\n");
    printf("║" E385(5m) " ");

    RENDER(m->registers[REGISTER_A], E(1m) E385(5m) "%X " E0(35m),
           E385(11m) "%X ");
    RENDER(m->registers[REGISTER_B], E(1m) E385(5m) "%X " E0(35m),
           E385(11m) "%X ");
    RENDER(m->registers[REGISTER_C], E(1m) E385(5m) "%X " E0(35m),
           E385(11m) "%X ");
    RENDER(m->registers[REGISTER_D], E(1m) E385(5m) "%X " E0(35m),
           E385(11m) "%X ");
    RENDER(m->registers[REGISTER_E], E(1m) E385(5m) "%X " E0(35m),
           E385(11m) "%X ");
    RENDER(m->registers[REGISTER_F], E(1m) E385(5m) "%X " E0(35m),
           E385(11m) "%X ");

    printf(E385(13m) "│" E385(4m) " ");

    printf(m->flags & FLAG_HALT ? E385(5m) "✺" E385(4m) : E385(11m) "◌");
    printf(m->flags & FLAG_INTERRUPT ? E385(5m) "✺" E385(4m) : E385(11m) "◌");
    printf(m->flags & FLAG_OVERFLOW ? E385(5m) "✺" E385(4m) : E385(11m) "◌");
    printf(m->flags & FLAG_CARRY ? E385(5m) "✺" E385(4m) : E385(11m) "◌");
    printf(m->flags & FLAG_ZERO ? E385(5m) "✺" E385(4m) : E385(11m) "◌");
    printf(m->flags & FLAG_NEGATIVE ? E385(5m) "✺" E385(4m) : E385(11m) "◌");

    printf(" " E385(13m) "│" E385(5m));

    RENDER((uint16_t)(m->pc - m->memory->data),
           " " E(1m) E385(5m) "%04X" E0(35m) " ", " " E385(11m) "%04X ");
    RENDER((uint16_t)(m->sp - m->memory->data),
           " " E(1m) E385(5m) "%04X" E0(35m) " ", " " E385(11m) "%04X ");
    RENDER((uint16_t)(m->iv - m->memory->data),
           " " E(1m) E385(5m) "%04X" E0(35m) " ", " " E385(11m) "%04X ");
    RENDER((uint16_t)(m->ix - m->memory->data),
           " " E(1m) E385(5m) "%04X" E0(35m) " ", " " E385(11m) "%04X ");
    RENDER((uint16_t)(m->ta - m->memory->data),
           " " E(1m) E385(5m) "%04X" E0(35m) " ", " " E385(11m) "%04X ");

    printf(E385(13m) "║ \n");
    printf("╠═════════╤═══╧════════╧══════════════════════════════╣ \n");

    printf("║ ");
    printf("%-1X ", m->memory->data[0xF000]);
    printf("%-1X ", m->memory->data[0xF001]);
    printf("%-1X ", m->memory->data[0xF002]);
    printf("%-1X ", m->memory->data[0xF003]);
    printf("│                                           ║\n");
    printf("║ ");
    printf("%-1X ", m->memory->data[0xF004]);
    printf("%-1X ", m->memory->data[0xF005]);
    printf("%-1X ", m->memory->data[0xF006]);
    printf("%-1X ", m->memory->data[0xF007]);
    printf("│                                           ║\n");
    printf("║ ");
    printf("%-1X ", m->memory->data[0xF008]);
    printf("%-1X ", m->memory->data[0xF009]);
    printf("%-1X ", m->memory->data[0xF00A]);
    printf("%-1X ", m->memory->data[0xF00B]);
    printf("│                                           ║\n");
    printf("║ ");
    printf("%-1X ", m->memory->data[0xF00C]);
    printf("%-1X ", m->memory->data[0xF00D]);
    printf("%-1X ", m->memory->data[0xF00E]);
    printf("%-1X ", m->memory->data[0xF00F]);
    printf("│                                           ║ \n");

    printf("╚═════════╧═══════════════════════════════════════════╝\n");
    printf(E(0m) "\n" E(?25h));
}

void sim_io(machine *m) {
    // Get the currently pressed keys. If they changed since last time and the
    // interrupt flag is not asserted, set the keyboard value in the
    // memory-mapped IO segment and assert the interrupt flag.
    char meta = 0;
    uint16_t keymap = kbio_get_keymap(&meta);

    if (meta == 'q') {
        m->flags |= 0x20; // Set halt flag
        return;
    }

    if (!(keymap == prev_keymap || m->flags & FLAG_INTERRUPT || m->int_mask)) {
        m->memory->data[0xFFF0] = (keymap & 0x000F) >> 0;
        m->memory->data[0xFFF1] = (keymap & 0x00F0) >> 4;
        m->memory->data[0xFFF2] = (keymap & 0x0F00) >> 8;
        m->memory->data[0xFFF3] = (keymap & 0xF000) >> 12;

        prev_keymap = keymap;
        m->flags |= FLAG_INTERRUPT;
    }
}
