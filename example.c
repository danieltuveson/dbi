#include <stdio.h>
#include <dbi.h>
#include <unistd.h>

// void dbi_register_command(DbiProgram prog, char *name, DbiForeignCall call, int argc);

// typedef enum DbiStatus (*DbiForeignCall)(DbiRuntime dbi);

enum DbiStatus hello(DbiRuntime dbi)
{
    char **context = dbi_get_context(dbi);
    printf("%s i: %d!\n", *context, dbi_get_var(dbi, 'a')->bint);
    *context = "hello from the old context!";
    return DBI_STATUS_GOOD;
}

enum DbiStatus ffi_sleep(DbiRuntime dbi)
{
    int argc = dbi_get_argc(dbi);
    struct DbiObject **argv = dbi_get_argv(dbi);
    if (argv[0]->type != DBI_INT) {
        printf("Error: expected int value in SLEEPFF\n");
        return DBI_STATUS_ERROR;
    }
    sleep(argv[0]->bint);
    return DBI_STATUS_GOOD;
}

int main(int argc, char *argv[])
{
    DbiProgram prog = dbi_program_new();

    dbi_register_command(prog, "HELLOFFI", hello, 0);
    dbi_register_command(prog, "SLEEPFFI", ffi_sleep, 1);

    bool ret = dbi_compile_file(prog, argv[1]);
    if (!ret) {
        printf("%s", dbi_strerror());
        dbi_program_free(prog);
        return 0;
    }

    struct DbiObject bob = { DBI_INT, { .bint = 123 } };
    char *context = "hello from context!";
    for (int i = 0; i < 2; i++) {
        DbiRuntime dbi = dbi_runtime_new();
        dbi_set_context(dbi, &context);
        bob.bint = i;
        dbi_set_var(dbi, 'a', &bob);
        if (!dbi_run(dbi, prog)) {
            printf("%s", dbi_strerror());
        }
        dbi_runtime_free(dbi);
    }

    dbi_program_free(prog);

    if (ret) {
        printf("All good\n");
        return 0;
    }
    return 1;
}

