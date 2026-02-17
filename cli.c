#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include "aux.h"
#include "dbi.h"

// Could just as easily replace this with a `!`, but that gets a little confusing
#define status(val) val ? EXIT_SUCCESS : EXIT_FAILURE

void print_cli_help(void)
{
    printf("Usage: dbi [options] \n"
            "Options:\n"
            "  -c file    compile file and print resulting bytecode\n"
            "  -e file    execute file\n"
            // "  -r file    execute file and start repl\n"
          );
}

const char *bad_input =
"Error: unknown argument %s\n"
"Run `dbi -h` for s list of valid arguments\n";

int main(int argc, char *argv[])
{
    DbiProgram prog = dbi_program_new();
    aux_register_commands(prog);

    int ret = EXIT_FAILURE;
    if (argc < 2) {
        ret = status(dbi_repl(prog, NULL));
    } else if (argc == 2) {
        if (strcmp(argv[1], "-h") == 0) {
            print_cli_help();
        } else if (argv[1][0] != '-') {
            ret = status(dbi_repl(prog, argv[1]));
            if (ret == EXIT_FAILURE) {
                printf("%s", dbi_strerror());
            }
        } else {
            printf(bad_input, argv[1]);
        }
    } else if (argc == 3) {
        if (strcmp(argv[1], "-c") == 0) {
            ret = status(dbi_compile_file(prog, argv[2]));
            if (ret == EXIT_FAILURE) {
                printf("%s", dbi_strerror());
            } else {
                dbi_print_compiled(prog);
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
        // } else if (strcmp(argv[1], "-r") == 0) {
        //     ret = status(dbi_repl(prog, argv[2]));
        } else {
            printf(bad_input, argv[1]);
        }
    } else {
        printf("Error: invalid arguments\n");
    }
    dbi_program_free(prog);
    return ret;
}

