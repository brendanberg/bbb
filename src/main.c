// #include <unistd.h>
#include "assem/assem.h"
#include "machine/cpu.h"
#include "machine/sim.h"
#include <memory.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_ADDRESS (64 * 1024)
#define BUFFER_SIZE 1024
#define USAGE_STRING                                                           \
    "usage: %s assemble SOURCE_FILE IMAGE\n       %s inspect IMAGE\n       "   \
    "%s run IMAGE\n"

void bbb_event_update(machine *m) {
    sim_print(m);
    sim_io(m);
}

void bbb_event_setup(machine *m) { sim_setup(m); }

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

    if (mem) {
        fwrite(mem->data, mem->size, 1, image);
        fflush(image);
        free(prog);

        return EXIT_SUCCESS;
    } else {
        fprintf(stderr, "error: unable to build image\n");
        return EXIT_FAILURE;
    }
}

int bbb_inspect(char *image_name) {
    FILE *pipe;
    char buffer[BUFFER_SIZE];
    char command[BUFFER_SIZE];

    int len = snprintf(
        command, BUFFER_SIZE,
        "hexdump -C %s | sed 's/ 0/ /g' | sed 'y/abcdef/ABCDEF/'", image_name);

    // Check if the command was truncated.
    if (len >= sizeof(command)) {
        perror("error: command length exceeds buffer size.");
        return EXIT_FAILURE;
    }

    pipe = popen(command, "r");

    if (pipe == NULL) {
        perror("error: unable to execute shell command");
        return EXIT_FAILURE;
    }

    while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
        printf("%s", buffer);
    }

    pclose(pipe);

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

    m->event_setup = bbb_event_setup;
    m->event_update = bbb_event_update;

    machine_start(m);
    machine_run(m);
    machine_free(m);

    return 0;
}

int main(int argc, char *argsv[]) {
    int status = EXIT_FAILURE;

    if (argc <= 2 || argc >= 6) {
        fprintf(stderr, USAGE_STRING, argsv[0], argsv[0], argsv[0]);
        return EXIT_FAILURE;
    }

    if (strcmp(argsv[1], "assemble") == 0) {
        if (argc < 4 || argc > 5) {
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

        if (status == EXIT_SUCCESS && argc == 5) {
            if (strcmp(argsv[4], "--inspect") == 0 ||
                strcmp(argsv[4], "-i") == 0) {
                status = bbb_inspect(image_path);
            }
        }

        return status;
    } else if (strcmp(argsv[1], "inspect") == 0) {
        if (argc != 3) {
            fprintf(stderr, "usage: %s inspect IMAGE\n", argsv[0]);
            return EXIT_FAILURE;
        }

        char *image_path = argsv[2];
        status = bbb_inspect(image_path);

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

    fprintf(stderr, USAGE_STRING, argsv[0], argsv[0], argsv[0]);
    return EXIT_FAILURE;
}
