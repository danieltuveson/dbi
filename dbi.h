#ifndef DBI_H
#define DBI_H

#include <stdint.h>
#include <stdbool.h>

// Should be set to *at least* whatever longest command name is (+1 character for null byte)
#define DBI_MAX_COMMAND_NAME 32 

// Arbitrary - adjust as needed
#define DBI_MAX_PROG_SIZE 10000
#define DBI_MAX_LINE_LENGTH 256 // Max number of chars that can be parsed in one line
#define DBI_MAX_STACK 128       // Max number of arithmatic expressions that can be on the stack
#define DBI_MAX_CALL_STACK 16   // Max depth of call stack (GOSUB's / RETURN)
#define DBI_MAX_LINE_MEMORY 64  // Max number of variables, numbers, or strings in one line
                                // NOTE: this should never be set to more than 256 since it will get
                                //       used as a uint8_t
#define DBI_MAX_BYTECODE 64
#define DBI_MAX_ITERATIONS 999999 // Maximum number of iterations of VM loop before aborting
#define DBI_MAX_ERROR 512

// Toggling turns on some debug printing
#define DBI_DEBUG 0

// Toggle to disable all commands that use IO
// Can be useful if embedding dbi into a non-cli application
#ifndef DBI_DISABLE_IO
#define DBI_DISABLE_IO 0
#endif

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
    DBI_STATUS_YIELD,
    DBI_STATUS_ERROR
};

typedef uintptr_t DbiProgram;
typedef uintptr_t DbiRuntime;

typedef enum DbiStatus (*DbiForeignCall)(DbiRuntime dbi);

// Allows C function to be called as a command
// If argc is -1, then the command can take one or more arguments. Otherwise, argc is the
// expected number of arguments for command
// The compiler will check if the correct number of arguments have been supplied before runtime.
// Command names *must* be unique and may not conflict with builtin command names
// `doc` can contain up to 50 characters of documentation text.
void dbi_register_command(DbiProgram prog, char *name, DbiForeignCall call, int argc);

// Note: all C function commands must be registered before compilation.
//       dbi_compile_* functions can be called multiple times with different inputs. If the line
//       number overlaps with an existing line, the existing line will be overwritten.
bool dbi_compile_file(DbiProgram prog, char *input_file_name);
bool dbi_compile_string(DbiProgram prog, char *text);

DbiProgram dbi_program_new(void);
void dbi_program_free(DbiProgram prog);

// Executes program in runtime
//
// If program finishes with DBI_STATUS_YIELD, calling dbi_run again will
// resume the program.
//
// Otherwise, if dbi_run is called the program will reset to the beginning
// (but retaining any local variables that were set)
enum DbiStatus dbi_run(DbiRuntime dbi, DbiProgram prog);

DbiRuntime dbi_runtime_new(void);
void dbi_runtime_free(DbiRuntime dbi);

// Writes an error message in the dbi runtime
// Should only be used for returning an error message from a foreign function
void dbi_runtime_error(DbiRuntime dbi, const char *fmt, ...);

// Get compilation / runtime errors as a string
// Error will be set after dbi_run is called
char *dbi_strerror(void);

// Context can be used to pass data between C and dbi in foreign calls
void dbi_set_context(DbiRuntime dbi, void *context);
void *dbi_get_context(DbiRuntime dbi);

// Get number of arguments + list of arguments from calling foreign command
int dbi_get_argc(DbiRuntime dbi);
struct DbiObject **dbi_get_argv(DbiRuntime dbi);

// Get object associated with var (which can be any letter a - z)
struct DbiObject *dbi_get_var(DbiRuntime dbi, char var);
void dbi_set_var(DbiRuntime dbi, char var, struct DbiObject *obj);

// If you just want to interactively run a BASIC script, use this (passing NULL for the file name
// just drops you into repl)
bool dbi_repl(DbiProgram prog, char *input_file_name);

// Get text of line at given number
char *dbi_get_line(DbiProgram prog, int lineno);

#endif

