// #include <unistd.h>
#include <memory.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "assem.h"
#include "cpu.h"

#define MAX_ADDRESS (64 * 1024)
#define USAGE_STRING "usage: %s assemble SOURCE_FILE IMAGE\n       %s run IMAGE\n"

int bbb_assemble(char *source_name, FILE *source, FILE *image) {
    if (fseek(source, 0L, SEEK_END) != 0) {
        fprintf(stderr, "error: unable to determine source file size\n");
        return EXIT_FAILURE;
    }

    size_t src_size = ftell(source);
    fseek(source, 0L, SEEK_SET);
    fseek(image, 0L, SEEK_SET);

    char *prog = calloc(src_size + 1, sizeof(char));
    fread(prog, sizeof(char), src_size, source);
    memory *mem = build_image(source_name, prog);

    fwrite(mem->data, mem->size, 1, image);
    fflush(image);
    free(prog);

    return EXIT_SUCCESS;
}

int bbb_run(FILE *image) {
    if (fseek(image, 0L, SEEK_END) != 0) {
        fprintf(stderr, "error: unable to determine image size\n");
        return EXIT_FAILURE;
    }

    size_t img_size = ftell(image);

    if (img_size > MAX_ADDRESS) {
        fprintf(stderr, "error: machine image is too big\n");
        return EXIT_FAILURE;
    }

    fseek(image, 0L, SEEK_SET);

    char *prog = calloc(img_size + 1, sizeof(char));
    fread(prog, sizeof(char), img_size, image);

    machine *m = machine_init(MAX_ADDRESS);
    memcpy(m->memory->data, prog, img_size);

    machine_start(m);
    machine_show(m);
    machine_run(m);
    machine_show(m);
    machine_free(m);

    return 0;
}

int main(int argc, char *argsv[]) {
    int status = EXIT_FAILURE;

    if (argc <= 2 || argc >= 5) {
        fprintf(stderr, USAGE_STRING, argsv[0], argsv[0]);
        return EXIT_FAILURE;
    }

    if (strcmp(argsv[1], "assemble") == 0) {
        if (argc != 4) {
            fprintf(stderr, "usage: %s assemble SOURCE IMAGE\n", argsv[0]);
            return EXIT_FAILURE;
        }

        char *src_path = argsv[2];
        char *image_path = argsv[3];
        // TODO: validation, etc
        FILE *source = fopen(src_path, "r");
        FILE *image = fopen(image_path, "wb");

        if (!source) {
            fprintf(stderr, "error: could not open source file for reading");
            return EXIT_FAILURE;
        }

        if (!image) {
            fprintf(stderr, "error: could not open image file for writing");
            return EXIT_FAILURE;
        }

        status = bbb_assemble(src_path, source, image);

        fclose(source);
        fclose(image);

        return status;
    } else if (strcmp(argsv[1], "run") == 0) {
        if (argc != 3) {
            fprintf(stderr, "usage: %s run IMAGE\n", argsv[0]);
            return EXIT_FAILURE;
        }

        char *image_path = argsv[2];
        // TODO: validate image path
        FILE *image = fopen(image_path, "rb");

        if (!image) {
            fprintf(stderr, "error: could not open the image file for reading");
            return EXIT_FAILURE;
        }
        status = bbb_run(image);

        fclose(image);

        return status;
    }

    fprintf(stderr, USAGE_STRING, argsv[0], argsv[0]);
    return EXIT_FAILURE;
}
