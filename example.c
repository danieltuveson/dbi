/*
 * This file includes various example uses of the DBI foreign function interface:
 * 1. Simple "Hello, world!" from FFI
 * 2. A more complex example using function arguments and error handling
 * 3. A function that accepts multiple arguments of different types
 * 4. Example of returning control to C program without ending execution
 * 5. Extending the language by adding an error handling mechanism
 */
#include <stdio.h>
#include <dbi.h>
#include <unistd.h>
#include <assert.h>

#define ignore(thing) (void)thing

// *******************************************************************
// ************************* Hello, world! *************************** 
// *******************************************************************
char *hello_program =
    "01 hello\n"
    "02 end\n";

enum DbiStatus hello_ffi(DbiRuntime dbi)
{
    ignore(dbi);
    printf("Hello from the FFI!\n");
    return DBI_STATUS_GOOD;
}

void example_hello_world(void)
{
    DbiProgram prog = dbi_program_new();

    // Register command HELLOFFI that calls C function hello_ffi, with zero arguments
    dbi_register_command(prog, "HELLO", hello_ffi, 0);

    bool ret = dbi_compile_string(prog, hello_program);
    assert(ret);

    DbiRuntime dbi = dbi_runtime_new();

    enum DbiStatus status = dbi_run(dbi, prog);
    assert(status == DBI_STATUS_FINISHED);

    dbi_runtime_free(dbi);
    dbi_program_free(prog);
}

// *******************************************************************
// **************** Function with arguments / errors ***************** 
// *******************************************************************
char *sleep_program_good =
    "01 print \"sleeping...\"\n"
    "02 sleepffi 1\n"
    "03 print \"awake\"\n"
    "04 end\n";

char *sleep_program_compile_err = "01 sleepffi 1, 2, 3\n";
char *sleep_program_runtime_err = "01 sleepffi \"this should be a runtime error\"\n";

enum DbiStatus sleep_ffi(DbiRuntime dbi)
{
    struct DbiObject **argv = dbi_get_argv(dbi);
    if (argv[0]->type != DBI_INT) {
        dbi_runtime_error(dbi, "expected numeric value in SLEEPFFI but got a string");
        return DBI_STATUS_ERROR;
    }
    sleep(argv[0]->bint);
    return DBI_STATUS_GOOD;
}

void example_sleep(void)
{
    char *programs[] = {
        sleep_program_good,
        sleep_program_compile_err,
        sleep_program_runtime_err
    };
    for (int i = 0; i < 3; i++) {
        char *program = programs[i];

        DbiProgram prog = dbi_program_new();
        dbi_register_command(prog, "SLEEPFFI", sleep_ffi, 1);

        bool ret = dbi_compile_string(prog, program);
        if (!ret) {
            printf("%s", dbi_strerror());
            dbi_program_free(prog);
            continue;
        }
        DbiRuntime dbi = dbi_runtime_new();
        enum DbiStatus status = dbi_run(dbi, prog);
        if (status == DBI_STATUS_ERROR) {
            printf("%s", dbi_strerror());
        }
        dbi_runtime_free(dbi);
        dbi_program_free(prog);
    }
}

// *******************************************************************
// **************** Function with multiple arguments ***************** 
// *******************************************************************
char *slow_print_program = "01 let x = 2 : slowprint 3, x, 1, \"blastoff!\" : end\n";

enum DbiStatus slow_print_ffi(DbiRuntime dbi)
{
    int argc = dbi_get_argc(dbi);
    struct DbiObject **argv = dbi_get_argv(dbi);
    for (int i = 0; i < argc; i++) {
        if (argv[i]->type == DBI_INT) {
            printf("%d", argv[i]->bint);
        } else {
            printf("%s", argv[i]->bstr);
        }
        sleep(1);
        fflush(stdout);
        if (i != argc - 1) printf(", ");
    }
    printf("\n");
    return DBI_STATUS_GOOD;
}

void example_slow_print(void)
{
    DbiProgram prog = dbi_program_new();

    // Register command HELLOFFI that calls C function hello_ffi, with zero arguments
    dbi_register_command(prog, "SLOWPRINT", slow_print_ffi, -1);

    bool ret = dbi_compile_string(prog, slow_print_program);
    assert(ret);

    DbiRuntime dbi = dbi_runtime_new();

    enum DbiStatus status = dbi_run(dbi, prog);
    assert(status == DBI_STATUS_FINISHED);

    dbi_runtime_free(dbi);
    dbi_program_free(prog);
}


int main(int argc, char *argv[])
{
    example_hello_world();
    example_sleep();
    example_slow_print();
    return 0;
}

// enum DbiStatus do_a_little_stuff(DbiRuntime dbi)
// {
//     char **context = dbi_get_context(dbi);
//     *context = "dolittle called";
//     return DBI_STATUS_YIELD;
// }
// 
// enum DbiStatus hello_ffi(DbiRuntime dbi)
// {
//     char **context = dbi_get_context(dbi);
//     printf("%s i: %d!\n", *context, dbi_get_var(dbi, 'a')->bint);
//     *context = "hello from the old context!";
//     return DBI_STATUS_GOOD;
// }

// {
//     if (argc != 2) {
//         printf("expected filename\n");
//         return 1;
//     }
//     DbiProgram prog = dbi_program_new();
// 
//     dbi_register_command(prog, "HELLOFFI", hello, 0);
//     dbi_register_command(prog, "SLEEPFFI", ffi_sleep, 1);
//     dbi_register_command(prog, "DOLITTLE", do_a_little_stuff, 0);
// 
//     bool ret = dbi_compile_file(prog, argv[1]);
//     if (!ret) {
//         printf("%s", dbi_strerror());
//         dbi_program_free(prog);
//         return 0;
//     }
// 
//     char *context = "hello from context!";
//     DbiRuntime dbi = dbi_runtime_new();
//     dbi_set_context(dbi, &context);
// 
//     enum DbiStatus status;
//     do {
//         status = dbi_run(dbi, prog);
//     } while (status == DBI_STATUS_YIELD);
//     if (status != DBI_STATUS_GOOD) {
//         printf("%s", dbi_strerror());
//     }
//     dbi_runtime_free(dbi);
//     // dbi_run(dbi, prog);
// 
//     // struct DbiObject bob = { DBI_INT, { .bint = 123 } };
//     // char *context = "hello from context!";
//     // for (int i = 0; i < 2; i++) {
//     //     DbiRuntime dbi = dbi_runtime_new();
//     //     dbi_set_context(dbi, &context);
//     //     bob.bint = i;
//     //     dbi_set_var(dbi, 'a', &bob);
//     //     if (!dbi_run(dbi, prog)) {
//     //         printf("%s", dbi_strerror());
//     //     }
//     //     dbi_runtime_free(dbi);
//     // }
// 
//     dbi_program_free(prog);
// 
//     if (ret) {
//         printf("All good\n");
//         return 0;
//     }
//     return 1;
// }

