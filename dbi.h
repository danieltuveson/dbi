#ifndef DBI_H
#define DBI_H

#include <stdint.h>
#include <stdbool.h>

// Hardcoded so max of 4 digit numbers for GOTOs and such - adjust as needed
#define DBI_MAX_PROG_SIZE 10000

// Should be set to *at least* whatever longest command name is (+1 character for null byte)
#define DBI_MAX_COMMAND_NAME 32 
#include <limits.h>

// Arbitrary - adjust as needed
#define DBI_MAX_LINE_LENGTH 256 // Max number of chars that can be parsed in one line
#define DBI_MAX_STACK 128       // Max number of arithmatic expressions that can be on the stack
#define DBI_MAX_CALL_STACK 16   // Max depth of call stack (GOSUB's / RETURN)
#define DBI_MAX_LINE_MEMORY 64  /* Max number of variables, numbers, or strings in one line
                                 * NOTE: this should never be set to more than 256 since it will get
                                 *       used as a uint8_t */
#define DBI_MAX_BYTECODE 64
#define DBI_MAX_ITERATIONS 999999 // Maximum number of iterations of VM loop before aborting

// Toggling turns on some debug printing
#define DBI_DEBUG 0

// Can disable specific commands, if desired, by setting to DBI_UNDEFINED
// Some commands like "END" are required for basic functionality and should not be disabled
// List below might be helpful for a sandboxed subset of commands if we want to compile
// arbitrary user code
// #define DBI_INPUT  DBI_UNDEFINED
// #define DBI_CLEAR  DBI_UNDEFINED 
// #define DBI_SLEEP  DBI_UNDEFINED
// #define DBI_QUOTE  DBI_UNDEFINED
// #define DBI_PRINT  DBI_UNDEFINED
// #define DBI_LIST   DBI_UNDEFINED
// #define DBI_LOAD   DBI_UNDEFINED
// #define DBI_BIG    DBI_UNDEFINED
// #define DBI_SAVE   DBI_UNDEFINED
// #define DBI_SYSTEM DBI_UNDEFINED
// #define DBI_BEEP   DBI_UNDEFINED
// #define DBI_HELP   DBI_UNDEFINED

enum DbiType {
    DBI_INT,
    DBI_STR,
    DBI_VAR
};

struct DbiObject {
    enum DbiType type;
    union {
        int bint;
        char *bstr;
        uint8_t bvar;
    };
};

enum DbiStatus {
    DBI_STATUS_GOOD,
    DBI_STATUS_FINISHED,
    DBI_STATUS_LOAD,
    DBI_STATUS_ERROR
};

typedef uintptr_t DbiProgram;
typedef uintptr_t DbiRuntime;

typedef enum DbiStatus (*DbiForeignCall)(DbiRuntime dbi);

// Allows C function to be called as a command
// If argc is -1, then the command can take one or more arguments. Otherwise, argc is the
// expected number of arguments for command
// Command names *must* be unique and may not conflict with builtin command names
void dbi_register_command(DbiProgram prog, char *name, DbiForeignCall call, int argc);

// You may only call dbi_compile once per program
// All foreign commands must be registered before compilation
bool dbi_compile(DbiProgram prog, char *input_file_name);
DbiProgram dbi_program_new(void);
void dbi_program_free(DbiProgram prog);

// Executes program in runtime
enum DbiStatus dbi_run(DbiRuntime dbi, DbiProgram prog);
DbiRuntime dbi_runtime_new(void);
void dbi_runtime_free(DbiRuntime dbi);

// Context can be used to pass data between C and dbi in foreign calls
void dbi_register_context(DbiRuntime dbi, void *context);

// Helper functions to be called from foreign call
void *dbi_get_context(DbiRuntime dbi);
int dbi_get_argc(DbiRuntime dbi);
struct DbiObject **dbi_get_argv(DbiRuntime dbi);

// Return object associated with var
struct DbiObject *dbi_get_var(DbiRuntime dbi, char var);

// If you just want to interactively run a BASIC script, use this (NULL just drops you into repl)
bool dbi_repl(char *input_file_name);

#endif

