// #include <unistd.h>
#include <memory.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "assemble.h"
#include "cpu.h"

#define MAX_ADDRESS (64 * 1024)

int bbb_assemble(FILE *source, FILE *image) {
    if (fseek(source, 0L, SEEK_END) != 0) {
        fputs("blah", stderr);
        return EXIT_FAILURE;
    }

    size_t src_size = ftell(source);
    fseek(source, 0L, SEEK_SET);

    if (fseek(image, 0L, SEEK_END) != 0) {
        fputs("blah", stderr);
        return EXIT_FAILURE;
    }

    size_t img_size = ftell(image);
    fseek(image, 0L, SEEK_SET);

    char *prog = calloc(src_size + 1, sizeof(char));
    fread(prog, sizeof(char), src_size, source);
    memory *mem = assemble(prog);

    fwrite(mem->data, mem->size, 1, image);
    fflush(image);

    return EXIT_SUCCESS;
}

int bbb_run(FILE *image) {
    if (fseek(image, 0L, SEEK_END) != 0) {
        fputs("blah", stderr);
        return EXIT_FAILURE;
    }

    size_t img_size = ftell(image);

    if (img_size >= MAX_ADDRESS) {
        fputs("blah", stderr);
        return EXIT_FAILURE;
    }

    fseek(image, 0L, SEEK_SET);

    char *prog = calloc(img_size + 1, sizeof(char));
    fgets(prog, img_size, image);
    machine *m = machine_init(MAX_ADDRESS);
    memcpy(m->memory->data, prog, img_size);

    machine_show(m);
    machine_run(m);
    machine_free(m);

    return 0;
}

int main(int argc, char *argsv[]) {
    int status = EXIT_FAILURE;

    if (strcmp(argsv[1], "assemble") == 0 && argc == 4) {
        char *src_path = argsv[2];
        char *image_path = argsv[3];
        // TODO: validation, etc
        FILE *source = fopen(src_path, "r");
        FILE *image = fopen(image_path, "wb");

        status = bbb_assemble(source, image);

        fclose(source);
        fclose(image);

        return status;
    } else if (strcmp(argsv[1], "run") == 0 && argc == 3) {
        char *image_path = argsv[2];
        // TODO: validate image path
        FILE *image = fopen(image_path, "rb");

        status = bbb_run(image);

        fclose(image);

        return status;
    }

    // TODO: Display help message
    return EXIT_FAILURE;
}
