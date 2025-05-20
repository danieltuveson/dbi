#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "dbi.h"

int main(int argc, char *argv[])
{
    if (argc < 2) {
        return !dbi_repl(NULL);
    } else if (argc == 2) {
        if (argv[1][0] == '-') {
            printf("Error: expected file name\n");
            return 0;
        }
        return !dbi_repl(argv[1]);
    } else if (argc == 3) {
        if (strcmp(argv[1], "-c") == 0) {
            DbiProgram prog = dbi_program_new();
            if (!dbi_compile_file(prog, argv[2])) {
                printf("%s", dbi_strerror());
                return 1;
            }
            dbi_program_free(prog);
            return 0;
        } else if (strcmp(argv[1], "-e") == 0) {
            DbiProgram prog = dbi_program_new();
            if (!dbi_compile_string(prog, argv[2])) {
                printf("%s", dbi_strerror());
                return 1;
            }
            DbiRuntime dbi = dbi_runtime_new();
            if (!dbi_run(dbi, prog)) {
                printf("%s", dbi_strerror());
                dbi_program_free(prog);
                return 1;
            }
            dbi_runtime_free(dbi);
            dbi_program_free(prog);
            return 0;
        } else if (strcmp(argv[1], "-r") == 0) {
            return !dbi_repl(argv[2]);
        } else {
            printf("Error: unknown argument %s\n", argv[1]);
            return 0;
        }
    }
    printf("Error: invalid arguments\n");
    return 1;
}

