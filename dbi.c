// https://en.wikipedia.org/wiki/Tiny_BASIC
// http://www.ittybittycomputers.com/ittybitty/tinybasic/TBuserMan.txt

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <stdbool.h>
#include <errno.h>
#include <unistd.h>
#include "pres.c"

// Hardcoded so max of 4 digit numbers for GOTOs and such
#define MAX_PROG_SIZE 10000

// Hardcoded since variables can be A-Z
#define MAX_VARS 26

// Arbitrary - adjust as needed
#define MAX_INPUT 256
#define MAX_STACK 128
#define MAX_CALL_STACK 128
#define MAX_LINE_MEMORY 32 // NOTE: this should never be set to more than 256
                           //       since it will get used as a uint8_t
#define MAX_BYTECODE 32

// Toggling turns on some debug printing
#define DEBUG 0

// This is the only mutable global variable. It is just used for making error messages nice.
static int global_lineno = 0;

static void compile_error(const char *fmt, ...)
{
    if (global_lineno <= 0) {
        printf("Error: ");
    } else {
        printf("Error at line %d: ", global_lineno);
    }
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    printf("\n");
}

// *******************************************************************
// ***************************** Commands ****************************
// *******************************************************************

enum Command {
    UNDEFINED,
    PRINT,
    IF,
    GOTO,
    INPUT,
    LET,
    GOSUB,
    RETURN,
    CLEAR,
    LIST,
    RUN,
    END,
    REM,
    LOAD,
    SAVE,
    BEEP,
    SLEEP,
    BIG,
    SYSTEM,
    HELP,
    QUOTE,
};

struct CommandMapping {
    char *str;
    enum Command command;
    char *helpstr;
    char *helpex;
};

struct CommandMapping command_map[] = {
    { "PRINT",  PRINT,  "print concatenated expression list",                 "PRINT expr-list" },
    { "IF",     IF,     "conditionally execute statement",       "IF expr relop expr THEN stmt" },
    { "GOTO",   GOTO,   "jump to given line number",                          "GOTO expr" },
    { "INPUT",  INPUT,  "get user input(s) and assign to variable(s)",        "INPUT var-list" },
    { "LET",    LET,    "set variable to expression",                         "LET var = expr" },
    { "GOSUB",  GOSUB,  "jump to given line number",                          "GOSUB expr" },
    { "RETURN", RETURN, "return to the line following the last GOSUB called", "RETURN" },
    { "CLEAR",  CLEAR,  "delete loaded code",                                 "CLEAR" },
    { "LIST",   LIST,   "print out loaded code",                              "LIST" },
    { "RUN",    RUN,    "execute loaded code",                                "RUN" },
    { "END",    END,    "end execution of program",                           "END" },
    { "REM",    REM,    "adds a comment",                                     "REM comment" },
    { "LOAD",   LOAD,   "load code from file",                                "LOAD string" },
    { "SAVE",   SAVE,   "save code to file",                                  "SAVE string" },
    { "BEEP",   BEEP,   "rings the bell",                                     "BEEP" },
    { "SLEEP",  SLEEP,  "sleeps for number of seconds",                       "SLEEP expr" },
    { "BIG",    BIG,    "toggles text output size",                           "BIG" },
    { "SYSTEM", SYSTEM, "run terminal command",                               "SYSTEM string" },
    { "QUOTE",  QUOTE,  "an inspirational quote",                             "QUOTE" },
    { "HELP",   HELP,   "you just ran this",                                  "HELP" },
};

int command_map_size = sizeof(command_map) / sizeof(*command_map);

struct GrammarMapping {
    char *symbol;
    char *expression;
};

struct GrammarMapping grammar_map[] = {
    { "line",          "number stmt (: stmt)* NL | stmt (: stmt)* NL" },
    { "stmt",          "see 'usage' above" },
    { "cmd",           "one of the commands above" },
    { "expr-list",     "(string|expr) (, (string|expr) )*" },
    { "var-list",      "var (, var)*" },
    { "expr",          "(+|-|ε) term ((+|-) term)*" },
    { "term",          "factor ((*|/) factor)*" },
    { "factor",        "var | number | (expr)" },
    { "var",           "A | B | C ... | Y | Z" },
    { "number",        "digit digit*" },
    { "digit",         "0 | 1 | 2 | 3 | ... | 8 | 9" },
    { "relop",         "< (>|=|eps) | > (<|=|eps) | =" },
    { "string",        "\" string-char* \"" },
    { "string-char",   "non-quote, non-newline character" },
    { "comment",       "non-newline character" },
};

int grammar_map_size = sizeof(grammar_map) / sizeof(*grammar_map);

void print_line(void)
{
    for (int i = 0; i < 97; i++) {
        putchar('-');
    }
    putchar('\n');
}

void print_help(void)
{
    print_line();
    printf(" %-8s|  %-52s|  %-25s\n", "command", "description", "usage");
    for (int i = 0; i < 97; i++) {
        if (i == 9 || i == 64) {
            putchar('+');
        } else {
            putchar('-');
        }
    }
    putchar('\n');
    for (int i = 0; i < command_map_size; i++) {
        struct CommandMapping cm = command_map[i];
        printf(" %-8s|  %-52s|  %-25s\n", cm.str, cm.helpstr, cm.helpex);
    }
    print_line();
    for (int i = 0; i < grammar_map_size; i++) {
        struct GrammarMapping gm = grammar_map[i];
        printf("%-11s  ::=  %s\n", gm.symbol, gm.expression);
    }
    print_line();
    printf("This BASIC interpreter is loosely based on Dennis Allison's Tiny BASIC\n");
    printf("Most of the above grammar is taken from https://en.wikipedia.org/wiki/Tiny_BASIC\n");
    printf("Source code is available at https://github.com/danieltuveson/dbi\n");
    printf("Happy hacking!\n");
    print_line();
}

void print_intro(void)
{
    printf("dan's basic interpreter - Copyright (C) 2025 Daniel Tuveson\n");
    printf("press ctrl+d or type 'end' to exit\n");
    printf("type 'help' for a list of commands\n");
}

char *quote =
"\n\t\"It is practically impossible to teach good programming to students\n"
"\tthat have had a prior exposure to BASIC: as potential programmers\n"
"\tthey are mentally mutilated beyond hope of regeneration.\"\n"
"\t― Edsger Dijkstra\n";



// *******************************************************************
// ************************** Basic Objects **************************
// *******************************************************************

enum BobType {
    BOB_INT,
    BOB_STR,
    BOB_VAR
        // , BOB_ARR
};

struct BObject {
    enum BobType type;
    union {
        int bint;
        char *bstr;
        uint8_t bvar;
    };
};

static inline struct BObject *bint_new(int i)
{
    struct BObject *obj = malloc(sizeof(*obj));
    obj->type = BOB_INT;
    obj->bint = i;
    return obj;
}

static inline struct BObject *bstr_new(char *str, int len)
{
    struct BObject *obj = malloc(sizeof(*obj));
    obj->type = BOB_STR;
    char *copy = malloc(len + 1);
    memcpy(copy, str, len);
    copy[len] = '\0';
    obj->bstr = copy;
    return obj;
}

static inline struct BObject *bvar_new(char c)
{
    struct BObject *obj = malloc(sizeof(*obj));
    obj->type = BOB_VAR;
    obj->bvar = c;
    return obj;
}

static void bobj_free(struct BObject *obj)
{
    assert(obj);
    if (obj->type == BOB_STR) {
        free(obj->bstr);
    }
    free(obj);
}

void bobj_print(struct BObject *obj, struct BObject **vars, bool big_font)
{
    if (obj->type == BOB_VAR) {
        obj = vars[obj->bvar];
    }
    if (big_font) {
        size_t len;
        char *strbuff;
        if (obj->type == BOB_INT) {
            len = 3 * sizeof(int) + 2;
            strbuff = calloc(len, 1);
            snprintf(strbuff, len, "%d", obj->bint);
            print_big(strbuff);
            free(strbuff);

        } else if (obj->type == BOB_STR) {
            len = strlen(obj->bstr) + 1;
            strbuff = calloc(len, 1);
            snprintf(strbuff, len, "%s", obj->bstr);
            print_big(strbuff);
            free(strbuff);

        } else {
            printf("Internal error: unknown type in PRINT statement\n");
        }
    } else {
        if (obj->type == BOB_INT) {
            printf("%d", obj->bint);
        } else if (obj->type == BOB_STR) {
            printf("%s", obj->bstr);
        } else {
            printf("Internal error: unknown type in PRINT statement\n");
        }
    }
}

void bobj_println(struct BObject *obj, struct BObject **vars, bool big_font)
{
    // if (big_font) printf("\n");
    bobj_print(obj, vars, big_font);
    printf("\n");
}

// *******************************************************************
// ************************ Memory / Bytecode ************************
// *******************************************************************
struct Memory {
    int index;
    struct BObject **array;
};

struct Bytecode {
    int index;
    uint8_t *array;
};

bool memory_check(struct Memory *memory)
{
    if (memory->index >= MAX_LINE_MEMORY) {
        compile_error("cannot allocate more memroy");
        return false;
    }
    return true;
}

int memory_add(struct Memory *memory, struct BObject *obj)
{
    memory->array[memory->index++] = obj;
    return memory->index - 1;
}

// On success returns memory location where object was placed
int memory_add_int(struct Memory *memory, int i)
{
    if (!memory_check(memory)) {
        return -1;
    }
    return memory_add(memory, bint_new(i));
}

int memory_add_str(struct Memory *memory, char *str, int len)
{
    if (!memory_check(memory)) {
        return -1;
    }
    return memory_add(memory, bstr_new(str, len));
}

int memory_add_var(struct Memory *memory, char c)
{
    if (!memory_check(memory)) {
        return -1;
    }
    return memory_add(memory, bvar_new(c));
}

void memory_clear(struct Memory *memory)
{
    if (memory->index > 0) {
        for (int i = 0; i < memory->index; i++) {
            bobj_free(memory->array[i]);
            memory->array[i] = NULL;
        }
    }
}

void bytecode_add(struct Bytecode *bytecode, uint8_t byte)
{
    if (bytecode->index >= MAX_BYTECODE) {
        return;
    }
    bytecode->array[bytecode->index++] = byte;
}

// *******************************************************************
// ************************* Basic Statement ************************* 
// *******************************************************************

struct Statement {
    int lineno;
    char *line;
    // list of BObjects used by statement
    struct Memory *memory;
    struct Bytecode *bytecode;
};

struct Statement *statement_new(int lineno, char *input, struct Memory *memory,
        struct Bytecode *bytecode)
{
    struct Statement *stmt = malloc(sizeof(*stmt));

    // Line info
    stmt->lineno = lineno;
    stmt->line = malloc(strlen(input) + 1);
    strcpy(stmt->line, input);

    // Memory
    stmt->memory = malloc(sizeof(*stmt->memory));
    stmt->memory->index = memory->index;
    if (memory->index != 0) {
        int size = sizeof(*stmt->memory) * memory->index;
        stmt->memory->array = malloc(size);
        memcpy(stmt->memory->array, memory->array, size);
    } else {
        stmt->memory->array = NULL;
    }

    // Bytecode
    stmt->bytecode = malloc(sizeof(*stmt->bytecode));
    stmt->bytecode->index = bytecode->index;
    if (bytecode->index != 0) {
        stmt->bytecode->array = malloc(bytecode->index);
        memcpy(stmt->bytecode->array, bytecode->array, bytecode->index);
    } else {
        stmt->bytecode->array = NULL;
    }
    return stmt;
}

void statement_free(struct Statement *stmt)
{
    free(stmt->line);
    if (stmt->memory) {
        memory_clear(stmt->memory);
        free(stmt->memory->array);
        free(stmt->memory);
    }
    if (stmt->bytecode) {
        free(stmt->bytecode->array);
        free(stmt->bytecode);
    }
    free(stmt);
}

// *******************************************************************
// *********************** Parsing / Compiling *********************** 
// *******************************************************************

enum Opcode {
    // No-op
    OP_NO,
    
    // Control flow / IO
    OP_PUSH,
    OP_PRINT,
    OP_PRINTLN,
    OP_JMP, // Jumps to line
    OP_JNZ, // Technnically this jumps to an opcode within a line, not an actual line
    OP_CALL,
    OP_INPUT,
    OP_LET,
    OP_RETURN,

    // Meta programming operations
    OP_CLEAR,
    OP_LIST,
    OP_RUN,
    OP_END,
    OP_LOAD,
    OP_SAVE,
    OP_HELP,
    OP_SLEEP,
    OP_BIG,
    OP_SYSTEM,

    // Comparison operators
    OP_LT,
    OP_GT,
    OP_EQ,
    OP_NEQ,
    OP_LEQ,
    OP_GEQ,

    // Math operators
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV
};

void ignore_whitespace(char **input_ptr)
{
    char *input = *input_ptr;
    while (isspace(*input)) input++;
    *input_ptr = input;
}

// Checks if input might be variable name
bool prefix_var(char c)
{
    return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z');
}

// Checks if input might be numeric expression
bool prefix_expr(char c)
{
    return prefix_var(c) || c == '+' || c == '-' || c == '(' || isdigit(c);
}

bool prefix_stmt_end(char c)
{
    return c == '\0' || c == ':';
}

bool prefix_op(char c)
{
    return c == '+' || c == '-' || c == '*' || c == '/';
}

// returns number of chars consumed
// -1 on error
int parse_lineno(char *input, int *lineno)
{
    if (!isdigit(*input)) {
        *lineno = 0;
        return 0;
    }
    *lineno = 0;
    int i = 0;
    while (isdigit(input[i])) {
        if (i > 4) {
            compile_error("line number exceeds maximum value of %d", MAX_PROG_SIZE - 1);
            global_lineno = -1;
            return -1;
        }
        *lineno = *lineno * 10 + input[i] - '0';
        i++;
    }
    if (*lineno == 0) {
        compile_error("line number cannot be 0");
        global_lineno = -1;
        return -1;
    }
    global_lineno = *lineno;
    return i;
}

// Returns number of chars consumed
int parse_command_name(char *input, enum Command *command_ptr)
{
    char command[7] = {0};
    int i = 0;
    while (i < 7 && prefix_var(input[i])) {
        command[i] = toupper(input[i]);
        i++;
    }
    for (int j = 0; j < command_map_size; j++) {
        int len = strlen(command_map[j].str);
        if (i == len && strncmp(command, command_map[j].str, len) == 0) {
            *command_ptr = command_map[j].command;
            return i;
        }
    }
    compile_error("unknown command");
    return 0;
}

// Returns number of chars consumed
int compile_string(char *input, struct Memory *memory, struct Bytecode *bytecode)
{
    input++; // discard opening quote
    char *str_start = input;
    while (*input != '"') {
        if (*input == '\0') {
            compile_error("unexpected end of string");
            return 0;
        }
        input++;
    }

    // Add string to statement memory
    int mem_loc = memory_add_str(memory, str_start, input - str_start);
    if (mem_loc == -1) {
        return 0;
    }

    // Add location of string object to bytecode
    bytecode_add(bytecode, OP_PUSH);
    bytecode_add(bytecode, mem_loc);

    return input - str_start + 2;
}

int compile_int(char *input, struct Memory *memory, struct Bytecode *bytecode)
{
    char *init_input = input;
    int num = 0;
    while (isdigit(*input)) {
        num = 10 * num + *input - '0';
        input++;
    }

    int mem_loc = memory_add_int(memory, num);
    if (mem_loc == -1) {
        return 0;
    }

    // Add location of object to bytecode
    bytecode_add(bytecode, OP_PUSH);
    bytecode_add(bytecode, mem_loc);
    return input - init_input;
}

int compile_var(char *input, struct Memory *memory, struct Bytecode *bytecode)
{
    uint8_t var = *input >= 'a' ? *input - 'a' : *input - 'A';
    input++;

    int mem_loc = memory_add_var(memory, var);
    if (mem_loc == -1) {
        return 0;
    }

    // Add location of object to bytecode
    bytecode_add(bytecode, OP_PUSH);
    bytecode_add(bytecode, mem_loc);
    return 1;
}

void push_op(char *stack, int *op_stack_offset, char op)
{
    *op_stack_offset = *op_stack_offset + 1;
    stack[*op_stack_offset] = op;
}

char pop_op(char *stack, int *op_stack_offset)
{
    char op = stack[*op_stack_offset];
    *op_stack_offset = *op_stack_offset - 1;
    return op;
}

#define push(op) push_op(stack, &op_stack_offset, op)
#define pop(op) pop_op(stack, &op_stack_offset)

#define peek()\
    op_stack_offset > 0 ? stack[op_stack_offset] : 0

// TODO: Make this handle non-literal expressions
// Need to add error code for when this can allocate multiple slots in memory / multiple
// bytes in the bytecode array
int compile_expr(char *input, struct Memory *memory, struct Bytecode *bytecode)
{
    char *init_input = input;
    int chars_parsed = 0;

    // int sign = 1;
    // if (*input == '-') {
    //     sign = -1;
    //     input++;
    // } else if (*input == '+') {
    //     input++;
    // }

    // Shunting yard
    char stack[MAX_STACK];
    int op_stack_offset = 0;
    int mode_op = 0;
    char op = 0;
    while (*input != '\0') {
        ignore_whitespace(&input);

        if (mode_op) {
            if (op_stack_offset + 1 >= MAX_STACK) {
                compile_error("expression exhausted operator stack");
                return 0;
            } else if (prefix_op(*input)) {
                op = peek();
#if DEBUG
                printf("op: %c\n", *input);
#endif
                if (op == '*' || op == '/') {
                    while (op != '+' && op != '-' && op != '(' && op != 0) {
                        pop();
                        op = peek();
                        if (op == '*') {
                            bytecode_add(bytecode, OP_MUL);
                        } else if (op == '/') {
                            bytecode_add(bytecode, OP_DIV);
                        }
                    }
                } else if (op == '+' || op == '-') {
                    while (op != '(' && op != 0) {
                        pop();
                        op = peek();
                        if (op == '*') {
                            bytecode_add(bytecode, OP_MUL);
                        } else if (op == '/') {
                            bytecode_add(bytecode, OP_DIV);
                        } else  if (op == '+') {
                            bytecode_add(bytecode, OP_ADD);
                        } else if (op == '-') {
                            bytecode_add(bytecode, OP_SUB);
                        }
                    }
                }
                push(*input);

            } else if (*input == '(') {
                push('(');

            } else if (*input == ')') {
            } else {
                break;
            }

            input++;
        } else {
            if (isdigit(*input)) {
                chars_parsed = compile_int(input, memory, bytecode);
            } else if (prefix_var(*input)) {
                chars_parsed = compile_var(input, memory, bytecode);
            } else {
                compile_error("expected number");
                return 0;
            }
            if (!chars_parsed) {
                return 0;
            }
            input += chars_parsed;
        }

        mode_op = !mode_op;
        // For now, just parse one thing
        // break;
    }

    while (peek() != 0) {
        op = pop();
        if (op == '(') {
            compile_error("unbalanced parentheses");
            return 0;
        } else if (op == '*') {
            bytecode_add(bytecode, OP_MUL);
        } else if (op == '/') {
            bytecode_add(bytecode, OP_DIV);
        } else  if (op == '+') {
            bytecode_add(bytecode, OP_ADD);
        } else if (op == '-') {
            bytecode_add(bytecode, OP_SUB);
        }
    }

    if (input == init_input) {
        compile_error("expected number or variable");
        return 0;
    }

    return input - init_input;
}

#undef push
#undef pop
#undef peek

// Returns number of chars parsed
int compile_expr_list(char *input, struct Memory *memory, struct Bytecode *bytecode)
{
    char *init_input = input;
    do {
        int char_count = 0;

        ignore_whitespace(&input);
        if (*input == '"') {
            char_count = compile_string(input, memory, bytecode);
        } else if (prefix_expr(*input)) {
            char_count = compile_expr(input, memory, bytecode);
        } else if (prefix_stmt_end(*input)) {
            compile_error("unexpected end of statement");
            return 0;
        } else {
            compile_error("invalid input: %s", input);
            return 0;
        }

        if (!char_count) {
            return 0;
        }
        bytecode_add(bytecode, OP_PRINT);
        input += char_count;

        ignore_whitespace(&input);
        if (*input == ',') {
            input++;
        } else {
            break;
        }
    } while (true);

    // Kind of hacky, but whatever
    bytecode->array[bytecode->index - 1] = OP_PRINTLN;

    return input - init_input;
}

int parse_relop(char *input, enum Opcode *op)
{
    if (*input == '<') {
        input++;
        if (*input == '>') {
            *op = OP_NEQ;
            return 2;
        } else if (*input == '=') {
            *op = OP_LEQ;
            return 2;
        }
        *op = OP_LT;
        return 1;
    } else if (*input == '>') {
        input++;
        if (*input == '<') {
            *op = OP_NEQ;
            return 2;
        } else if (*input == '=') {
            *op = OP_GEQ;
            return 2;
        }
        *op = OP_GT;
        return 1;
    } else if (*input == '=') {
        *op = OP_EQ;
        return 1;
    }
    compile_error("expected comparison operator");
    *op = OP_NO;
    return 0;
}

int compile_statement(char *input, struct Memory *memory, struct Bytecode *bytecode,
        int lineno, enum Command *command_ptr);

int compile_if(char *input, struct Memory *memory, struct Bytecode *bytecode,
        int lineno, enum Command *command_ptr)
{
    char *init_input = input;

    // Compile first expression
    int char_count = compile_expr(input, memory, bytecode);
    if (char_count == 0) {
        return 0;
    }
    input += char_count;
    ignore_whitespace(&input);

    // Parse comparison operator
    enum Opcode op;
    char_count = parse_relop(input, &op);
    if (char_count == 0) {
        return 0;
    }
    input += char_count;
    ignore_whitespace(&input);

    // Compile second expression
    char_count = compile_expr(input, memory, bytecode);
    if (char_count == 0) {
        return 0;
    }
    input += char_count;
    ignore_whitespace(&input);

    // Compile comparison operator
    bytecode_add(bytecode, op);

    // Bytecode location for JNZ to jump to.
    // Actually gets set a few lines down, once we know the memory offset.
    int mem_loc = memory_add_int(memory, 0);
    if (mem_loc == -1) {
        return 0;
    }
    bytecode_add(bytecode, OP_PUSH);
    bytecode_add(bytecode, mem_loc);
    bytecode_add(bytecode, OP_JNZ);

    // Parse "THEN" token
    char chars[5] = {0};
    for (int i = 0; i < 4; i++) {
        if (*input == '\0') {
            compile_error("unexpected end of input");
            return 0;
        }
        chars[i] = toupper(*input);
        input++;
    }
    ignore_whitespace(&input);

    if (strncmp(chars, "THEN", 4) != 0) {
        compile_error("expected 'THEN'");
        return 0;
    }

    // Parse statement
    char_count = compile_statement(input, memory, bytecode, lineno, command_ptr);
    if (!char_count) {
        return 0;
    }
    input += char_count;
    ignore_whitespace(&input);

    // Set location for above JNZ
    // Having it jump to a no-op, since there could be more statements on same line
    memory->array[mem_loc]->bint = bytecode->index;
    bytecode_add(bytecode, OP_NO);

    return input - init_input;
}

int compile_let(char *input, struct Memory *memory, struct Bytecode *bytecode)
{
    char *init_input = input;
    if (!prefix_var(*input)) {
        compile_error("expected variable name");
        return 0;
    }
    uint8_t var = *input >= 'a' ? *input - 'a' : *input - 'A';
    input++;

    ignore_whitespace(&input);
    if (*input != '=') {
        compile_error("missing '=' in LET statement");
        return 0;
    }
    input++;

    ignore_whitespace(&input);
    int chars_parsed = compile_expr(input, memory, bytecode);
    if (!chars_parsed) {
        return 0;
    }
    input += chars_parsed;

    bytecode_add(bytecode, OP_LET);
    bytecode_add(bytecode, var);
    return input - init_input;
}

int compile_statement(char *input, struct Memory *memory, struct Bytecode *bytecode,
        int lineno, enum Command *command_ptr)
{
    char *init_input = input;

    // Get command
    enum Command command;
    int chars_parsed = parse_command_name(input, &command);
    if (!chars_parsed) {
        return 0;
    }
    input += chars_parsed;
    ignore_whitespace(&input);
    *command_ptr = command;

    // Parse based on command
    int mem_loc;
    switch (command) {
        case PRINT:
            chars_parsed = compile_expr_list(input, memory, bytecode);
            if (!chars_parsed) {
                return 0;
            }
            input += chars_parsed;
            break;
        case IF:
            chars_parsed = compile_if(input, memory, bytecode, lineno, command_ptr);
            if (chars_parsed == 0) {
                return 0;
            }
            input += chars_parsed;
            break;
            // case INPUT:
            //     break;
        case LET:
            chars_parsed = compile_let(input, memory, bytecode);
            if (!chars_parsed) {
                return 0;
            }
            input += chars_parsed;
            break;
        case GOSUB:
            mem_loc = memory_add_int(memory, lineno + 1);
            if (mem_loc == -1) {
                return 0;
            }
            bytecode_add(bytecode, OP_PUSH);
            bytecode_add(bytecode, mem_loc);
            bytecode_add(bytecode, OP_CALL);
            /* fall through */
        case GOTO:
            chars_parsed = compile_expr(input, memory, bytecode);
            if (!chars_parsed) {
                return 0;
            }
            input += chars_parsed;
            bytecode_add(bytecode, OP_JMP);
            break;
        case RETURN:
            bytecode_add(bytecode, OP_RETURN);
            break;
        case CLEAR:
            bytecode_add(bytecode, OP_CLEAR);
            break;
        case LIST:
            bytecode_add(bytecode, OP_LIST);
            break;
        case RUN:
            bytecode_add(bytecode, OP_RUN);
            break;
        case END:
            bytecode_add(bytecode, OP_END);
            break;
        case REM:
            bytecode_add(bytecode, OP_NO);
            while (*input != '\0') input++;
            break;
        case LOAD:
        case SAVE:
            if (*input != '"') {
                compile_error("expected file name\n");
                return 0;
            }
            chars_parsed = compile_string(input, memory, bytecode);
            if (chars_parsed == 0) {
                return 0;
            }
            input += chars_parsed;
            bytecode_add(bytecode, command == LOAD ? OP_LOAD : OP_SAVE);
            break;
        case QUOTE:
            mem_loc = memory_add_str(memory, quote, strlen(quote));
            if (mem_loc == -1) {
                return 0;
            }
            bytecode_add(bytecode, OP_PUSH);
            bytecode_add(bytecode, mem_loc);
            bytecode_add(bytecode, OP_PRINTLN);
            break;
        case BEEP:
            mem_loc = memory_add_str(memory, "\a", strlen(quote));
            if (mem_loc == -1) {
                return 0;
            }
            bytecode_add(bytecode, OP_PUSH);
            bytecode_add(bytecode, mem_loc);
            bytecode_add(bytecode, OP_PRINTLN);
            break;
        case SLEEP:
            chars_parsed = compile_expr(input, memory, bytecode);
            if (!chars_parsed) {
                return 0;
            }
            input += chars_parsed;
            bytecode_add(bytecode, OP_SLEEP);
            break;
        case BIG:
            bytecode_add(bytecode, OP_BIG);
            break;
        case SYSTEM:
            if (*input != '"') {
                compile_error("expected command\n");
                return 0;
            }
            chars_parsed = compile_string(input, memory, bytecode);
            if (chars_parsed == 0) {
                return 0;
            }
            input += chars_parsed;
            bytecode_add(bytecode, OP_SYSTEM);
            break;
        case HELP:
            bytecode_add(bytecode, OP_HELP);
            break;
        default:
            printf("Command not implemented\n");
            return 0;
    }
    return input - init_input;
}

// Returns number of bytes in bytecode
struct Statement *compile_line(char *input, struct Memory *memory, struct Bytecode *bytecode)
{
    ignore_whitespace(&input);

    // Ignore empty lines
    if (*input == '\0') {
        return 0;
    }

    char *init_input = input;

    // Get line number
    int lineno = 0;
    int chars_parsed = parse_lineno(input, &lineno);
    if (chars_parsed == -1) {
        return NULL;
    }
    input += chars_parsed;

    // Compile statement(s)
    do {
        ignore_whitespace(&input);

        enum Command command;
        chars_parsed = compile_statement(input, memory, bytecode, lineno, &command);
        if (!chars_parsed) {
            memory_clear(memory);
            return NULL;
        }
        input += chars_parsed;

        ignore_whitespace(&input);
        if (*input == ':') {
            if (command == RUN) {
                compile_error("RUN must be last command in statement");
                memory_clear(memory);
                return NULL;
            } else if (command == LOAD) {
                compile_error("LOAD must be last command in statement");
                memory_clear(memory);
                return NULL;
            }
            input++;
        } else {
            break;
        }
    } while (true);

    // Technically someone could come *exactly* up against these limits without going over
    // but to make error checking simpler, I don't care
    if (bytecode->index == MAX_BYTECODE) {
        compile_error("generated code too large");
        memory_clear(memory);
        return 0;
    }
    if (memory->index == MAX_LINE_MEMORY) {
        compile_error("generated code exceeds memory usage limit");
        memory_clear(memory);
        return 0;
    }

    ignore_whitespace(&input);
    if (*input != '\0') {
        compile_error("unexpected input %c", *input);
        memory_clear(memory);
        return NULL;
    }
    return statement_new(lineno, init_input, memory, bytecode);
}

// *******************************************************************
// ************************* VM / Execution ************************** 
// *******************************************************************

enum Status {
    STATUS_GOOD,
    STATUS_FINISHED,
    STATUS_LOAD,
    STATUS_ERROR
};

void program_clear(struct Statement **program)
{
    for (int i = 0; i < MAX_PROG_SIZE; i++) {
        struct Statement *stmt = program[i];
        if (stmt) {
            statement_free(stmt);
        }
        program[i] = NULL;
    }
}

void program_list(struct Statement **program)
{
    for (int i = 0; i < MAX_PROG_SIZE; i++) {
        struct Statement *stmt = program[i];
        if (stmt) {
            printf("%s", stmt->line);
        }
    }
}

int program_save(struct Statement **program, char *filename)
{
    FILE *file = fopen(filename, "w+");
    if (!file) {
        return 0;
    }
    for (int i = 0; i < MAX_PROG_SIZE; i++) {
        struct Statement *stmt = program[i];
        if (stmt) {
            fprintf(file, "%s", stmt->line);
        }
    }
    fclose(file);
    return 1;
}

struct Statement *statement_next(struct Statement **program, int lineno)
{
    for (int i = lineno; i < MAX_PROG_SIZE; i++) {
        struct Statement *stmt = program[i];
        if (stmt) {
            return stmt;
        }
    }
    return NULL;
}

static void runtime_error(struct Statement *stmt, const char *fmt, ...)
{
    if (stmt == NULL || stmt->lineno == 0) {
        printf("Error: ");
    } else {
        printf("Error at line %d: ", stmt->lineno);
    }
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    printf("\n");
}

#define push(val)\
    memcpy(&(stack[++stack_offset]), val, sizeof(*val))

#define push_int(num) do {\
    stack_offset++;\
    stack[stack_offset].type = BOB_INT;\
    stack[stack_offset].bint = num;\
} while(0)

#define pop()\
    &(stack[stack_offset--])

#define push_sub(line)\
    callstack[++callstack_offset] = line

#define pop_sub()\
    callstack[callstack_offset--]

// Boilerplate checking for arithmatic expressions
#define math_boilerplate() do {\
    obj = pop();\
    if (obj->type == BOB_VAR) {\
        obj = vars[obj->bvar];\
    }\
    if (obj->type != BOB_INT) {\
        runtime_error(stmt, "non-integer in arithmatic expression");\
        return STATUS_ERROR;\
    }\
    rnum = obj->bint;\
    obj = pop();\
    if (obj->type == BOB_VAR) {\
        obj = vars[obj->bvar];\
    }\
    if (obj->type != BOB_INT) {\
        runtime_error(stmt, "non-integer in arithmatic expression");\
        return STATUS_ERROR;\
    }\
    lnum = obj->bint;\
} while(0)

enum Status execute_statement(
        struct Statement *stmt,
        struct BObject **vars,
        struct Statement **program,
        char **filename,
        bool *big_font)
{
    enum Status status = STATUS_GOOD;

    int stack_offset = 0;
    struct BObject stack[MAX_STACK];

    int callstack_offset = 0;
    int callstack[MAX_CALL_STACK];

    struct BObject *obj;
    int mem_loc;
    int ip = 0;
    int lnum, rnum;
    int cmp;

#if DEBUG
    printf("stmt->array->index:%d\n", stmt->bytecode->index);
    printf("stmt->memory->index:%d\n", stmt->memory->index);

    printf("mem {");
    for (int i = 0; i < stmt->memory->index; i++) {
        if (i != 0) printf(", ");
        struct BObject *obj = stmt->memory->array[i];
        if (obj->type == BOB_INT) {
            printf("%d", obj->bint);
        } else if (obj->type == BOB_STR) {
            printf("%s", obj->bstr);
        } else {
            printf("%c", obj->bvar);
        }
    }
    printf("}\n");

    printf("bytecode->array {");
    for (int i = 0; i < stmt->bytecode->index; i++) {
        if (i != 0) printf(", ");
        printf("%d", stmt->bytecode->array[i]);
    }
    printf("}\n");

#endif

    while (true) {
        uint8_t op = stmt->bytecode->array[ip];

#if DEBUG
        printf("op: %d\n", op);
#endif

        switch (op) {
            case OP_NO:
                break;
            case OP_PUSH:
                mem_loc = stmt->bytecode->array[++ip];
                if (stack_offset + 1 >= MAX_STACK) {
                    runtime_error(stmt, "stack overflow");
                    return STATUS_ERROR;
                }
                push(stmt->memory->array[mem_loc]);
                break;
            case OP_PRINT:
                obj = pop();
                bobj_print(obj, vars, *big_font);
                break;
            case OP_PRINTLN:
                obj = pop();
                bobj_println(obj, vars, *big_font);
                break;
                // case OP_IF:
                //     break;
            case OP_INPUT:
                break;
            case OP_LET:
                obj = pop();
                mem_loc = stmt->bytecode->array[++ip];
                if (obj->type == BOB_VAR) {
                    if (mem_loc != obj->bvar) {
                        memcpy(vars[mem_loc], vars[obj->bvar], sizeof(**vars));
                    }
                } else {
                    memcpy(vars[mem_loc], obj, sizeof(**vars));
                }
                break;
            case OP_JMP:
                obj = pop();
                if (obj->type == BOB_VAR) {
                    obj = vars[obj->bvar];
                }
                if (obj->type != BOB_INT) {
                    runtime_error(stmt, "cannot goto non-integer");
                    return STATUS_ERROR;
                } else if (obj->bint <= 0 || obj->bint >= MAX_PROG_SIZE) {
                    runtime_error(stmt, "goto %d out of bounds", obj->bint);
                    return STATUS_ERROR;
                } else if (program[obj->bint] == NULL) {
                    runtime_error(stmt, "cannot goto %d, no such line", obj->bint);
                    return STATUS_ERROR;
                }
                ip = 0;
                stmt = program[obj->bint];
                continue;
            case OP_JNZ:
                obj = pop();
                mem_loc = obj->bint;
                obj = pop();
                cmp = obj->bint; 
                if (!cmp) {
                    ip = mem_loc;
                }
                break;
            case OP_CALL:
                if (callstack_offset + 1 >= MAX_CALL_STACK) {
                    runtime_error(stmt, "stack overflow");
                    return STATUS_ERROR;
                }
                obj = pop();
                push_sub(obj->bint);
                break;
            case OP_RETURN:
                if (callstack_offset <= 0) {
                    runtime_error(stmt, "no outer subroutine to return to");
                    return STATUS_ERROR;
                }
                stmt = statement_next(program, pop_sub());
                if (stmt) {
                    ip = 0;
                    continue;
                } 
                return STATUS_GOOD;
            case OP_CLEAR:
                program_clear(program);
                break;
            case OP_LIST:
                program_list(program);
                break;
            case OP_RUN:
                stmt = statement_next(program, 0);
                if (!stmt) {
                    return STATUS_GOOD;
                }
                ip = 0;
                continue;
            case OP_END:
                return STATUS_FINISHED;
            case OP_LOAD:
                obj = pop();
                *filename = obj->bstr;
                return STATUS_LOAD;
            case OP_SAVE:
                obj = pop();
                *filename = obj->bstr;
                if (!program_save(program, *filename)) {
                    runtime_error(stmt, "%s", strerror(errno));
                }
                break;
            case OP_SLEEP:
                obj = pop();
                if (obj->type == BOB_VAR) {
                    obj = vars[obj->bvar];
                }
                if (obj->type != BOB_INT) {
                    runtime_error(stmt, "cannot use string value for SLEEP");
                    return STATUS_ERROR;
                }
                sleep(obj->bint);
                break;
            case OP_BIG:
                *big_font = !*big_font;
                break;
            case OP_SYSTEM:
                obj = pop();
                if (obj->type == BOB_VAR) {
                    obj = vars[obj->bvar];
                }
                if (obj->type != BOB_STR) {
                    runtime_error(stmt, "cannot use integer value for SYSTEM");
                    return STATUS_ERROR;
                }
                int err = system(obj->bstr);
                if (err == -1) {
                    runtime_error(stmt, "%s", strerror(errno));
                }
                break;
            case OP_HELP:
                print_help();
                break;
            case OP_ADD:
                math_boilerplate();
                push_int(lnum + rnum);
                break;
            case OP_SUB:
                math_boilerplate();
                push_int(lnum - rnum);
                break;
            case OP_MUL:
                math_boilerplate();
                push_int(lnum * rnum);
                break;
            case OP_DIV:
                math_boilerplate();
                if (rnum == 0) {
                    runtime_error(stmt, "division by zero");
                }
                push_int(lnum / rnum);
                break;
            case OP_LT:
                math_boilerplate();
                push_int(lnum < rnum);
                break;
            case OP_GT:
                math_boilerplate();
                push_int(lnum > rnum);
                break;
            case OP_EQ:
                math_boilerplate();
                // printf("lnum:%d; rnum:%d\n", lnum, rnum);
                push_int(lnum == rnum);
                break;
            case OP_NEQ:
                math_boilerplate();
                push_int(lnum != rnum);
                break;
            case OP_LEQ:
                math_boilerplate();
                push_int(lnum <= rnum);
                break;
            case OP_GEQ:
                math_boilerplate();
                push_int(lnum >= rnum);
                break;
            default:
                printf("Internal error: unknown command encountered\n");
                return STATUS_ERROR;
        }
        ip++;
        if (ip >= stmt->bytecode->index) {
            if (stmt->lineno == 0) {
                // If one-off command, exit
                break;
            }
            stmt = statement_next(program, stmt->lineno + 1);
            if (!stmt) {
                break;
            }
            ip = 0;
        }
    }
    return status;
}

void vars_init(struct BObject **vars)
{
    for (int i = 0; i < MAX_VARS; i++) {
        vars[i] = malloc(sizeof(**vars));
        vars[i]->type = BOB_INT;
        vars[i]->bint = 0;
    }
}

void vars_free(struct BObject **vars)
{
    for (int i = 0; i < MAX_VARS; i++) {
        free(vars[i]);
    }
}

int main(void)
{
    print_intro();

    struct BObject *vars[MAX_VARS] = {0};
    vars_init(vars);

    char input[MAX_INPUT];

    uint8_t temp_bytecode_array[MAX_BYTECODE];
    struct Bytecode temp_bytecode = { 0, temp_bytecode_array };

    struct BObject *temp_memory_array[MAX_LINE_MEMORY];
    struct Memory temp_memory = { 0, temp_memory_array };

    struct Statement **program = calloc(MAX_PROG_SIZE, sizeof(*program));

    FILE *file = stdin;
    char *filename;

    bool input_error = false;
    bool big_font = false;

    while (true) {
        memset(input, 0, MAX_INPUT);

        temp_bytecode.index = 0;
        memset(temp_bytecode_array, 0, MAX_BYTECODE);

        temp_memory.index = 0;
        memset(temp_memory_array, 0, sizeof(*temp_memory_array) * MAX_LINE_MEMORY);

        global_lineno = -1;

        if (file == stdin && !input_error) {
            printf("> ");
        }

        if (!fgets(input, MAX_INPUT, file)) {
            if (file == stdin) {
                printf("\n");
                break;
            } else {
                fclose(file);
                file = stdin;
            }
        }

        // If user gives a line that exceeds length, ignore fgets input until we're parsed the
        // whole line
        if (input[MAX_INPUT - 2] != '\0') {
            if (!input_error) {
                printf("Error: input line too long \n");
            }
            input_error = true;
            continue;
        } else if (input_error) {
            input_error = false;
            continue;
        }

        struct Statement *stmt = compile_line(input, &temp_memory, &temp_bytecode);
        if (!stmt) {
            // Error
            continue;
        } else if (stmt->lineno == 0) {
            // No line number means we execute the command immediately
            enum Status status = execute_statement(stmt, vars, program, &filename, &big_font);
            if (status == STATUS_FINISHED) {
                statement_free(stmt);
                break;
            } else if (status == STATUS_LOAD) {
                if (file != stdin) {
                    fclose(file);
                }
                file = fopen(filename, "r");
                if (!file) {
                    runtime_error(stmt, "%s", strerror(errno));
                    file = stdin;
                }
            }
            statement_free(stmt);
        } else {
            if (program[stmt->lineno]) {
                statement_free(program[stmt->lineno]);
            }
            program[stmt->lineno] = stmt;
        }
    }
    if (file != stdin) {
        fclose(file);
    }

    vars_free(vars);
    program_clear(program);
    free(program);
    return 0;
}

