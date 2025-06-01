#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include "dbi.h"

// Could just as easily replace this with a `!`, but that gets a little confusing
#define status(val) val ? EXIT_SUCCESS : EXIT_FAILURE

int main(int argc, char *argv[])
{
    DbiProgram prog = dbi_program_new();
    int ret = EXIT_FAILURE;
    if (argc < 2) {
        ret = status(dbi_repl(prog, NULL));
    } else if (argc == 2) {
        ret = status(dbi_repl(prog, argv[1]));
        if (ret == EXIT_FAILURE) {
            printf("%s", dbi_strerror());
        }
    } else if (argc == 3) {
        if (strcmp(argv[1], "-c") == 0) {
            ret = status(dbi_compile_file(prog, argv[2]));
            if (ret == EXIT_FAILURE) {
                printf("%s", dbi_strerror());
            }
        } else if (strcmp(argv[1], "-e") == 0) {
            ret = status(dbi_compile_file(prog, argv[2]));
            if (ret == EXIT_FAILURE) {
                printf("%s", dbi_strerror());
            } else {
                DbiRuntime dbi = dbi_runtime_new();
                ret = status(dbi_run(dbi, prog));
                if (ret == EXIT_FAILURE) {
                    printf("%s", dbi_strerror());
                }
                dbi_runtime_free(dbi);
            }
        } else if (strcmp(argv[1], "-r") == 0) {
            ret = status(dbi_repl(prog, argv[2]));
        } else {
            printf("Error: unknown argument %s\n", argv[1]);
        }
    } else {
        printf("Error: invalid arguments\n");
    }
    dbi_program_free(prog);
    return ret;
}

