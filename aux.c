#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include "dbi.h"
#include "aux.h"
#include "./extras/bigtext.c"

#define IGNORE(arg) ((void)arg)

static enum DbiStatus aux_system(DbiRuntime dbi)
{
    int argc = dbi_get_argc(dbi);
    assert(argc == 1);
    struct DbiObject **argv = dbi_get_argv(dbi);
    if (argv[0]->type != DBI_STR) {
        dbi_runtime_error(dbi, "expected string argument for SYSTEM command");
        return DBI_STATUS_ERROR;
    }
    system(argv[0]->bstr);
    return DBI_STATUS_GOOD;
}

static void aux_big_print_obj(struct DbiObject *obj)
{
    size_t len;
    char *strbuff;
    if (obj->type == DBI_INT) {
        len = 3 * sizeof(int) + 2;
        strbuff = calloc(len, 1);
        snprintf(strbuff, len, "%ld", obj->bint);
    } else if (obj->type == DBI_STR) {
        len = strlen(obj->bstr) + 1;
        strbuff = calloc(len, 1);
        snprintf(strbuff, len, "%s", obj->bstr);
    } else {
        assert(false && "Internal runtime error: unknown type in BIG statement");
    }
    print_big(strbuff);
    free(strbuff);
}

// Basically a copy of aux_print but calling aux_big_print_obj
static enum DbiStatus aux_big(DbiRuntime dbi)
{
    int argc = dbi_get_argc(dbi);
    if (argc < 1) {
        dbi_runtime_error(dbi, "BIG command requires at least 1 argument");
        return DBI_STATUS_ERROR;
    }
    struct DbiObject **argv = dbi_get_argv(dbi);
    for (int i = 0; i < argc; i++) {
        struct DbiObject *obj = argv[i];
        aux_big_print_obj(obj);
        fflush(stdout);
    }
    return DBI_STATUS_GOOD;
}

static enum DbiStatus aux_sleep(DbiRuntime dbi)
{
    int argc = dbi_get_argc(dbi);
    assert(argc == 1);
    struct DbiObject **argv = dbi_get_argv(dbi);
    if (argv[0]->type != DBI_INT) {
        dbi_runtime_error(dbi, "expected integer argument for SLEEP command");
        return DBI_STATUS_ERROR;
    }
    sleep(argv[0]->bint);
    return DBI_STATUS_GOOD;
}

static void aux_print_obj(struct DbiObject *obj)
{
    if (obj->type == DBI_INT) {
        printf("%ld", obj->bint);
    } else if (obj->type == DBI_STR) {
        printf("%s", obj->bstr);
    } else {
        assert(false && "Internal runtime error: unknown type in PRINT statement");
    }
}

static enum DbiStatus aux_print(DbiRuntime dbi)
{
    int argc = dbi_get_argc(dbi);
    if (argc < 1) {
        dbi_runtime_error(dbi, "PRINT command requires at least 1 argument");
        return DBI_STATUS_ERROR;
    }
    struct DbiObject **argv = dbi_get_argv(dbi);
    for (int i = 0; i < argc; i++) {
        struct DbiObject *obj = argv[i];
        aux_print_obj(obj);
        fflush(stdout);
    }
    printf("\n");
    return DBI_STATUS_GOOD;
}

static enum DbiStatus aux_beep(DbiRuntime dbi)
{
    int argc = dbi_get_argc(dbi);
    assert(argc == 0);
    printf("\a\n");
    return DBI_STATUS_GOOD;
}

static enum DbiStatus aux_quote(DbiRuntime dbi)
{
    IGNORE(dbi);
    printf( "\n\t\"It is practically impossible to teach good programming to students\n"
            "\tthat have had a prior exposure to BASIC: as potential programmers\n"
            "\tthey are mentally mutilated beyond hope of regeneration.\"\n"
            "\t― Edsger Dijkstra\n\n");
    return DBI_STATUS_GOOD;
}

void aux_register_commands(DbiProgram prog)
{
    dbi_register_command_with_info(prog, "QUOTE",  aux_quote,  0,  "an inspirational quote",             "QUOTE");
    dbi_register_command_with_info(prog, "BEEP",   aux_beep,   0,  "rings the bell",                     "BEEP");
    dbi_register_command_with_info(prog, "SLEEP",  aux_sleep,  1,  "sleeps for number of seconds",       "SLEEP int");
    dbi_register_command_with_info(prog, "SYSTEM", aux_system, 1,  "run terminal command",               "SYSTEM string");
    dbi_register_command_with_info(prog, "PRINT",  aux_print,  -1, "print concatenated expression list", "PRINT expr-list");
    dbi_register_command_with_info(prog, "BIG",    aux_big,    -1, "print embiggened text",              "BIG expr-list");
}

