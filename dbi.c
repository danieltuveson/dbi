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
#include "dbi.h"

#if FF_BIG
#include "bigtext.c"
#endif

#if FF_SLEEP
#include <unistd.h>
#endif

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
#if FF_SLEEP
    SLEEP,
#endif
#if FF_BIG
    BIG,
#endif
    SYSTEM,
    GETENV,
    SETENV,
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
#if FF_SLEEP
    { "SLEEP",  SLEEP,  "sleeps for number of seconds",                   "SLEEP expr" },
#endif
#if FF_BIG
    { "BIG",    BIG,    "toggles text embiggening",                       "BIG" },
#endif
    { "SYSTEM", SYSTEM, "run terminal command",                           "SYSTEM string" },
    { "GETENV", GETENV, "get environment variable",                       "GETENV var = string" },
    { "SETENV", SETENV, "set environment variable",                       "SETENV string, string" },
    { "QUOTE",  QUOTE,  "an inspirational quote",                         "QUOTE" },
    { "HELP",   HELP,   "you just ran this",                              "HELP" },
};

int command_map_size = sizeof(command_map) / sizeof(*command_map);

static char *command_to_str(enum Command command)
{
    for (int i = 0; i < command_map_size; i++) {
        if (command_map[i].command == command) {
            return command_map[i].str;
        }
    }
    return NULL;
}

struct GrammarMapping {
    char *symbol;
    char *expression;
};

struct GrammarMapping grammar_map[] = {
    { "line",          "number stmt (: stmt)* NL | stmt (: stmt)* NL" },
    { "stmt",          "see 'usage' above" },
    { "cmd",           "one of the commands above" },
    { "expr-list",     "expr (, expr)*" },
    { "var-list",      "var (, var)*" },
    { "expr",          "term ((+|-) term)*" },
    { "term",          "factor ((*|/) factor)*" },
    { "factor",        "var | number | string | (expr)" },
    { "var",           "A | B | C ... | Y | Z" },
    { "number",        "(+|-|eps) digit digit*" },
    { "digit",         "0 | 1 | 2 | 3 | ... | 8 | 9" },
    { "relop",         "< (>|=|eps) | > (<|=|eps) | =" },
    { "string",        "\" string-char* \"" },
    { "string-char",   "non-quote, non-newline character" },
    { "comment",       "non-newline character" },
    { "eps",           "nothing"},
};

int grammar_map_size = sizeof(grammar_map) / sizeof(*grammar_map);

static void print_line(void)
{
    for (int i = 0; i < 97; i++) {
        putchar('-');
    }
    putchar('\n');
}

static void print_help(void)
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
    printf("Most of the above grammar is taken from page 9 of Dr. Dobb's Journal:\n"
            "https://archive.org/download/dr_dobbs_journal_vol_01/dr_dobbs_journal_vol_01.pdf\n");
    printf("Source code is available at https://github.com/danieltuveson/dbi\n");
    printf("Happy hacking!\n");
    print_line();
}

static void print_intro(void)
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
};

struct BObject {
    enum BobType type;
    union {
        int bint;
        char *bstr;
        uint8_t bvar;
    };
};

static struct BObject *bint_new(int i)
{
    struct BObject *obj = malloc(sizeof(*obj));
    obj->type = BOB_INT;
    obj->bint = i;
    return obj;
}

static struct BObject *bstr_new(char *str, int len)
{
    char *copy = malloc(len + 1);
    memcpy(copy, str, len);
    copy[len] = '\0';

    struct BObject *obj = malloc(sizeof(*obj));
    obj->type = BOB_STR;
    obj->bstr = copy;
    return obj;
}

static void bobj_copy(struct BObject *dest, struct BObject *src)
{
    if (src->type == BOB_INT) {
        dest->type = BOB_INT;
        dest->bint = src->bint;
    } else if (src->type == BOB_STR) {
        if (dest->type == BOB_STR) {
            free(dest->bstr);
        }
        dest->type = BOB_STR;
        int len = strlen(src->bstr);
        char *copy = malloc(len + 1);
        memcpy(copy, src->bstr, len);
        copy[len] = '\0';
        dest->bstr = copy;
    } else {
        assert(0);
    }
}

static struct BObject *bvar_new(char c)
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

static bool memory_check(struct Memory *memory)
{
    if (memory->index >= MAX_LINE_MEMORY) {
        compile_error("cannot allocate more memroy");
        return false;
    }
    return true;
}

static int memory_add(struct Memory *memory, struct BObject *obj)
{
    memory->array[memory->index++] = obj;
    return memory->index - 1;
}

// On success returns memory location where object was placed
static int memory_add_int(struct Memory *memory, int i)
{
    if (!memory_check(memory)) {
        return -1;
    }
    return memory_add(memory, bint_new(i));
}

static int memory_add_str(struct Memory *memory, char *str, int len)
{
    if (!memory_check(memory)) {
        return -1;
    }
    return memory_add(memory, bstr_new(str, len));
}

static int memory_add_var(struct Memory *memory, char c)
{
    if (!memory_check(memory)) {
        return -1;
    }
    return memory_add(memory, bvar_new(c));
}

static void memory_clear(struct Memory *memory)
{
    if (memory->index > 0) {
        for (int i = 0; i < memory->index; i++) {
            bobj_free(memory->array[i]);
            memory->array[i] = NULL;
        }
    }
}

static void bytecode_add(struct Bytecode *bytecode, uint8_t byte)
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

static struct Statement *statement_new(int lineno, char *input, struct Memory *memory,
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

static void statement_free(struct Statement *stmt)
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
#if FF_SLEEP
    OP_SLEEP,
#endif
#if FF_BIG
    OP_BIG,
#endif
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

static void ignore_whitespace(char **input_ptr)
{
    char *input = *input_ptr;
    while (isspace(*input)) input++;
    *input_ptr = input;
}

// Checks if input might be variable name
static bool prefix_var(char c)
{
    return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z');
}

static uint8_t get_var(char c)
{
    return c >= 'a' ? c - 'a' : c - 'A';
}

static bool prefix_number(char c)
{
    return c == '+' || c == '-' || isdigit(c);
}

// Checks if input might be numeric expression
static bool prefix_expr(char c)
{
    return prefix_number(c) || prefix_var(c) || c == '(' || c == '"';
}

static bool prefix_stmt_end(char c)
{
    return c == '\0' || c == ':';
}

static bool prefix_op(char c)
{
    return c == '+' || c == '-' || c == '*' || c == '/';
}

// returns number of chars consumed
// -1 on error
static int parse_lineno(char *input, int *lineno)
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
static int parse_command_name(char *input, enum Command *command_ptr)
{
    char command[MAX_COMMAND_NAME] = {0};
    int i = 0;
    while (i < MAX_COMMAND_NAME && prefix_var(input[i])) {
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
static int compile_string(char *input, struct Memory *memory, struct Bytecode *bytecode)
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

static int compile_int(char *input, struct Memory *memory, struct Bytecode *bytecode)
{
    char *init_input = input;

    int sign = 1;
    if (*input == '-') {
        sign = -1;
        input++;
    } else if (*input == '+') {
        input++;
    }
    ignore_whitespace(&input);

    int num = 0;
    while (isdigit(*input)) {
        num = 10 * num + sign * (*input - '0');
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

static int compile_var(char *input, struct Memory *memory, struct Bytecode *bytecode)
{
    uint8_t var = get_var(*input);
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

static bool push_op(char *stack, int *op_stack_offset, char op)
{
    if (*op_stack_offset + 1 >= MAX_STACK) {
        compile_error("large expression exhausted operator stack");
        return false;
    }
#if DEBUG
    printf("calling push op with '%c',", op);
#endif
    *op_stack_offset = *op_stack_offset + 1;
#if DEBUG
    printf("stack offset of '%d'\n", *op_stack_offset);
#endif
    stack[*op_stack_offset] = op;
    return true;
}

static char pop_op(char *stack, int *op_stack_offset)
{
    char op = stack[*op_stack_offset];
    *op_stack_offset = *op_stack_offset - 1;
    return op;
}


static void compile_op(struct Bytecode *bytecode, char op)
{
    if (op == '*') {
        bytecode_add(bytecode, OP_MUL);
    } else if (op == '/') {
        bytecode_add(bytecode, OP_DIV);
    } else  if (op == '+') {
        bytecode_add(bytecode, OP_ADD);
    } else if (op == '-') {
        bytecode_add(bytecode, OP_SUB);
    } else {
#if DEBUG
        printf("op: %c\n", op);
#endif
        assert(0);
    }
}

#define push(op) push_op(stack, &op_stack_offset, op)
#define pop(op) pop_op(stack, &op_stack_offset)

#define peek()\
    op_stack_offset > 0 ? stack[op_stack_offset] : 0

#if DEBUG
static void print_stack(char stack[MAX_STACK], int stack_offset)
{
    printf("stack {");
    for (int i = 0; i < stack_offset; i++) {
        printf("'%c'", stack[i]);
        if (i + 1 != stack_offset) {
            printf(", ");
        }
    }
    printf("}\n");
}
#endif

static int compile_expr(char *input, struct Memory *memory, struct Bytecode *bytecode)
{
    char *init_input = input;
    int chars_parsed = 0;

    // shunting yard
    char stack[MAX_STACK];
    int op_stack_offset = 0;
    int mode_op = 0;
    char op = 0;
    while (*input != '\0') {
        ignore_whitespace(&input);

        if (mode_op) {
            if (*input == ')') {
                while (*input == ')') {
                    op = peek();
                    if (op == 0) {
                        compile_error("closing parenthesis does not match any opening parenthesis");
                        return 0;
                    }
                    while (op != '(' && op != 0) {
                        compile_op(bytecode, op);
                        pop();
                        op = peek();
                    }
                    if (op != '(') {
                        compile_error("opening parenthesis does not match any closing parenthesis");
                        return 0;
                    }
                    pop();
#if DEBUG
                    printf("peek after paren pop: %c\n", peek());
#endif
                    input++;
                    ignore_whitespace(&input);
                }
            }
            if (prefix_op(*input)) {
#if DEBUG
                printf("stack offset: %d\n", op_stack_offset);
                printf("current op: %c\n", *input);
                printf("peek: %c\n", peek());
#endif
                op = peek();
                if (*input == '*' || *input == '/') {
                    while (op != '+' && op != '-' && op != '(' && op != 0) {
                        compile_op(bytecode, op);
                        pop();
                        op = peek();
                    }
                } else if (*input == '+' || *input == '-') {
                    while (op != '(' && op != 0) {
                        compile_op(bytecode, op);
                        pop();
                        op = peek();
                    }
                }
                if (!push(*input)) {
                    return 0;
                }
                input++;
            } else {
#if DEBUG
                printf("breaking at '%c'\n", *input);
#endif
                break;
            }
        } else {
            while (*input == '(') {
                if (!push(*input)) {
                    return 0;
                }
                input++;
                ignore_whitespace(&input);
            }
            if (prefix_number(*input)) {
                chars_parsed = compile_int(input, memory, bytecode);
                if (!chars_parsed) {
                    return 0;
                }
                input += chars_parsed;
            } else if (*input == '"') {
                chars_parsed = compile_string(input, memory, bytecode);
                if (!chars_parsed) {
                    return 0;
                }
                input += chars_parsed;
            } else if (prefix_var(*input)) {
                chars_parsed = compile_var(input, memory, bytecode);
                if (!chars_parsed) {
                    return 0;
                }
                input += chars_parsed;
            } else {
                compile_error("expected number");
                return 0;
            }
        }
        mode_op = !mode_op;
#if DEBUG
        print_stack(stack, op_stack_offset);
#endif
    }

    op = peek();
    while (op != 0) {
        if (op == '(') {
            compile_error("unbalanced parentheses");
            return 0;
        }
        compile_op(bytecode, op);
        pop();
        op = peek();
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
static int compile_print(char *input, struct Memory *memory, struct Bytecode *bytecode)
{
    char *init_input = input;
    do {
        int char_count = 0;

        ignore_whitespace(&input);
        if (prefix_expr(*input)) {
            char_count = compile_expr(input, memory, bytecode);
        } else if (prefix_stmt_end(*input)) {
            compile_error("unexpected end of statement");
            return 0;
        } else {
            compile_error("invalid input: %c", *input);
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

static int parse_relop(char *input, enum Opcode *op)
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

static int compile_statement(char *input, struct Memory *memory, struct Bytecode *bytecode,
        int lineno, enum Command *command_ptr);

static int compile_if(char *input, struct Memory *memory, struct Bytecode *bytecode,
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
        if (prefix_stmt_end(*input)) {
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

static int compile_input(char *input, struct Bytecode *bytecode)
{
    char *init_input = input;
    bool comma = false;
    uint32_t flags = 0;
    uint8_t count = 0; // Can just put this in bytecode, since it's always less than MAX_VARS

    bytecode_add(bytecode, OP_INPUT);

    // Counter for number of input variables set after loop
    uint8_t count_index = bytecode->index;
    bytecode_add(bytecode, 0);

    while (true) {
        if (comma) {
            if (*input != ',') {
                break;
            }
        } else {
            if (!prefix_var(*input)) {
                compile_error("expected variable name");
                return 0;
            }

            uint8_t var = get_var(*input);
            if (flags & (UINT32_C(1) << var)) {
                compile_error("'%c' used twice in INPUT statement", *input);
                return 0;
            } else {
                flags = flags | (UINT32_C(1) << var);
            }

            bytecode_add(bytecode, var);
            count++;
        }
        input++;
        ignore_whitespace(&input);
        comma = !comma;
    }
    bytecode->array[count_index] = count;

    if (input == init_input) {
        compile_error("expected variable name");
        return 0;
    }
    return input - init_input;
}

static int compile_let(char *input, struct Memory *memory, struct Bytecode *bytecode)
{
    char *init_input = input;
    if (!prefix_var(*input)) {
        compile_error("expected variable name");
        return 0;
    }
    uint8_t var = get_var(*input);
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

static int compile_statement(char *input, struct Memory *memory, struct Bytecode *bytecode,
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
    if (command == SAVE || command == LOAD || command == SLEEP || command == SYSTEM) {
        chars_parsed = compile_expr(input, memory, bytecode);
        if (!chars_parsed) {
            return 0;
        }
        input += chars_parsed;
    }
    switch (command) {
        case PRINT:
            chars_parsed = compile_print(input, memory, bytecode);
            if (!chars_parsed) {
                return 0;
            }
            input += chars_parsed;
            break;
        case IF:
            chars_parsed = compile_if(input, memory, bytecode, lineno, command_ptr);
            if (!chars_parsed) {
                return 0;
            }
            input += chars_parsed;
            break;
        case INPUT:
            chars_parsed = compile_input(input, bytecode);
            if (!chars_parsed) {
                return 0;
            }
            input += chars_parsed;
            break;
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
            bytecode_add(bytecode, OP_LOAD);
            break;
        case SAVE:
            bytecode_add(bytecode, OP_SAVE);
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
#if FF_SLEEP
        case SLEEP:
            bytecode_add(bytecode, OP_SLEEP);
            break;
#endif
#if FF_BIG
        case BIG:
            bytecode_add(bytecode, OP_BIG);
            break;
#endif
        case SYSTEM:
            bytecode_add(bytecode, OP_SYSTEM);
            break;
        case HELP:
            bytecode_add(bytecode, OP_HELP);
            break;
        default:
            compile_error("command not implemented");
            return 0;
    }
    return input - init_input;
}

// Technically someone could come *exactly* up against these limits without going over
// but to make error checking simpler, I don't care
static bool end_of_user_input_checks(char *input, struct Bytecode *bytecode, struct Memory *memory)
{
    if (bytecode->index == MAX_BYTECODE) {
        compile_error("generated code too large");
        return false;
    }
    if (memory->index == MAX_LINE_MEMORY) {
        compile_error("generated code exceeds memory usage limit");
        return false;
    }

    ignore_whitespace(&input);
    if (*input != '\0') {
        compile_error("unexpected input %c", *input);
        return false;
    }
    return true;
}

// Returns number of bytes in bytecode
static struct Statement *compile_line(char *input, struct Memory *memory, struct Bytecode *bytecode)
{
    ignore_whitespace(&input);

    // Ignore empty lines and # comments
    if (*input == '\0' || *input == '#') {
        return NULL;
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
            if (command == RUN || command == INPUT || command == LOAD) {
                compile_error("%s must be last command in statement", command_to_str(command));
                memory_clear(memory);
                return NULL;
            }
            input++;
        } else {
            break;
        }
    } while (true);

    if (!end_of_user_input_checks(input, bytecode, memory)) {
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

static void program_clear(struct Statement **program)
{
    for (int i = 0; i < MAX_PROG_SIZE; i++) {
        struct Statement *stmt = program[i];
        if (stmt) {
            statement_free(stmt);
        }
        program[i] = NULL;
    }
}

static void program_list(struct Statement **program)
{
    for (int i = 0; i < MAX_PROG_SIZE; i++) {
        struct Statement *stmt = program[i];
        if (stmt) {
            printf("%s", stmt->line);
        }
    }
}

static int program_save(struct Statement **program, char *filename)
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

static struct Statement *statement_next(struct Statement **program, int lineno)
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

static void bobj_print(struct BObject *obj, struct BObject **vars, bool big_font)
{
    if (obj->type == BOB_VAR) {
        obj = vars[obj->bvar];
    }
#if FF_BIG
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
            runtime_error(NULL, "Internal runtime error: unknown type in PRINT statement");
        }
    } else {
#else
        (void) big_font; // Ignore parameter
#endif
        if (obj->type == BOB_INT) {
            printf("%d", obj->bint);
        } else if (obj->type == BOB_STR) {
            printf("%s", obj->bstr);
        } else {
            runtime_error(NULL, "Internal runtime error: unknown type in PRINT statement");
#if FF_BIG
        }
#endif
    }
}

static void bobj_println(struct BObject *obj, struct BObject **vars, bool big_font)
{
    bobj_print(obj, vars, big_font);
    printf("\n");
}

// Compile input into a bunch of OP_LETs - kinda hacky but I can't think of a better way
static struct Statement *execute_input(struct Statement *stmt, int var_count, uint8_t *var_list)
{
    global_lineno = stmt->lineno;
    char input_arr[MAX_LINE_LENGTH] = {0};
    char *input = input_arr; // Decay to pointer, please
    char *init_input = input;

    if (!fgets(input, MAX_LINE_LENGTH, stdin)) {
        compile_error("unexpected end of input");
        return NULL;
    }

    uint8_t temp_bytecode_array[MAX_BYTECODE] = {0};
    struct Bytecode temp_bytecode = { 0, temp_bytecode_array };

    struct BObject *temp_memory_array[MAX_LINE_MEMORY] = {0};
    struct Memory temp_memory = { 0, temp_memory_array };

    int current_var_count = 0;
    do {
        int char_count = 0;
        ignore_whitespace(&input);

        // Compile expression
        if (prefix_expr(*input)) {
            char_count = compile_expr(input, &temp_memory, &temp_bytecode);
            if (!char_count) {
                memory_clear(&temp_memory);
                return NULL;
            }
        } else if (*input == '\0') {
            compile_error("unexpected end of input");
            memory_clear(&temp_memory);
            return NULL;
        } else {
            compile_error("invalid input: %c", *input);
            memory_clear(&temp_memory);
            return NULL;
        }
        input += char_count;

        // Compile inserted LET
        if (current_var_count >= var_count) {
            compile_error("too many inputs values (expected %d)", var_count);
            memory_clear(&temp_memory);
            return NULL;
        }
        bytecode_add(&temp_bytecode, OP_LET);
        bytecode_add(&temp_bytecode, var_list[current_var_count]);

        ignore_whitespace(&input);
        if (*input == ',') {
            input++;
        } else if (*input == '\0') {
            break;
        }
        current_var_count++;
    } while (true);

    if (current_var_count < var_count - 1) {
        compile_error("expected %d input value(s), but got %d", var_count, current_var_count + 1);
        memory_clear(&temp_memory);
        return NULL;
    }

    if (!end_of_user_input_checks(input, &temp_bytecode, &temp_memory)) {
        memory_clear(&temp_memory);
        return NULL;
    }

    return statement_new(global_lineno, init_input, &temp_memory, &temp_bytecode);
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

#define expect_int(in) do {\
    if (obj->type == BOB_VAR) {\
        obj = vars[obj->bvar];\
    }\
    if (obj->type != BOB_INT) {\
        runtime_error(stmt, "expected integer %s", in);\
        return STATUS_ERROR;\
    }\
} while(0)

#define expect_string(in) do {\
    if (obj->type == BOB_VAR) {\
        obj = vars[obj->bvar];\
    }\
    if (obj->type != BOB_STR) {\
        runtime_error(stmt, "expected string %s", in);\
        return STATUS_ERROR;\
    }\
} while(0)

// Boilerplate checking for arithmatic expressions
#define math_boilerplate() do {\
    obj = pop();\
    expect_int("in arithmatic expression");\
    rnum = obj->bint;\
    obj = pop();\
    expect_int("in arithmatic expression");\
    lnum = obj->bint;\
} while(0)

static enum Status execute_line(
        struct Statement *stmt,
        struct BObject **vars,
        struct Statement **program,
        bool run_file,
        // Out parameters
        struct Statement **input_stmt_ptr,
        char **filename_ptr,
        bool *big_font_ptr)
{
    enum Status status = STATUS_GOOD;

    int stack_offset = 0;
    struct BObject stack[MAX_STACK];

    int callstack_offset = 0;
    int callstack[MAX_CALL_STACK];

    struct BObject *obj;
    int ip = 0;

    // Forward declarations since clang doesn't like these in switch
    int mem_loc, count;
    int lnum, rnum;
    int cmp;

    while (true) {
        uint8_t op = stmt->bytecode->array[ip];

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
                bobj_print(obj, vars, *big_font_ptr);
                break;
            case OP_PRINTLN:
                obj = pop();
                bobj_println(obj, vars, *big_font_ptr);
                break;
            case OP_INPUT:
                count = stmt->bytecode->array[++ip];

                // Clear out old input, if it exists
                if (*input_stmt_ptr != NULL) {
                    statement_free(*input_stmt_ptr);
                    *input_stmt_ptr = NULL;
                }
                
                // Get new input
                *input_stmt_ptr = execute_input(stmt, count, stmt->bytecode->array + ip + 1);
                if (*input_stmt_ptr == NULL) {
                    return STATUS_ERROR;
                }

                // Execute compiled input
                stmt = *input_stmt_ptr;
                ip = 0;
                continue;
            case OP_LET:
                obj = pop();
                mem_loc = stmt->bytecode->array[++ip];
                if (obj->type == BOB_VAR) {
                    if (mem_loc != obj->bvar) {
                        bobj_copy(vars[mem_loc], vars[obj->bvar]);
                    }
                } else {
                    bobj_copy(vars[mem_loc], obj);
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
                if (callstack_offset <=  0) {
                    // If we're not in a subroutine, this sends us back to the REPL
                    return STATUS_GOOD;
                }
                stmt = statement_next(program, pop_sub());
                if (stmt) {
                    ip = 0;
                    continue;
                } 
                return STATUS_GOOD;
            case OP_CLEAR:
                if (stmt->lineno != 0) {
                    // If statement is self-destructing, just return to REPL
                    program_clear(program);
                    return STATUS_GOOD;
                } else {
                    program_clear(program);
                }
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
                if (run_file || stmt->lineno == 0) {
                    return STATUS_FINISHED;
                }
                return STATUS_GOOD;
            case OP_LOAD:
                obj = pop();
                expect_string("argument for LOAD command");
                *filename_ptr = obj->bstr;
                return STATUS_LOAD;
            case OP_SAVE:
                obj = pop();
                expect_string("argument for SAVE command");
                if (!program_save(program, obj->bstr)) {
                    runtime_error(stmt, "%s", strerror(errno));
                    return STATUS_ERROR;
                }
                break;
#if FF_SLEEP
            case OP_SLEEP:
                obj = pop();
                expect_int("argument for SLEEP command");
                sleep(obj->bint);
                break;
#endif
#if FF_BIG
            case OP_BIG:
                *big_font_ptr = !*big_font_ptr;
                break;
#endif
            case OP_SYSTEM:
                obj = pop();
                expect_string("argument for SYSTEM command");
                int err = system(obj->bstr);
                if (err == -1) {
                    runtime_error(stmt, "%s", strerror(errno));
                    return STATUS_ERROR;
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
                    return STATUS_ERROR;
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
                runtime_error(stmt, "Internal error: unknown command encountered\n");
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

static void vars_init(struct BObject **vars)
{
    for (int i = 0; i < MAX_VARS; i++) {
        vars[i] = malloc(sizeof(**vars));
        vars[i]->type = BOB_INT;
        vars[i]->bint = 0;
    }
}

static void vars_free(struct BObject **vars)
{
    for (int i = 0; i < MAX_VARS; i++) {
        if (vars[i]->type == BOB_STR) {
            free(vars[i]->bstr);
        }
        free(vars[i]);
    }
}

// Loads provided file then executes RUN command
// If filename is NULL it will drop into the REPL and not execute RUN
//
// Probably will split some of this up so that it can be used in a more library-ish way
int dbi_run_file(char *input_file_name)
{
    bool run_file;
    FILE *file;

    if (input_file_name) {
        file = fopen(input_file_name, "r");
        run_file = true;
        if (!file) {
            runtime_error(NULL, "%s", strerror(errno));
            return 1;
        }
    } else {
        file = stdin;
        run_file = false;
        print_intro();
    }

    char *filename;

    struct BObject *vars[MAX_VARS] = {0};
    vars_init(vars);

    char input[MAX_LINE_LENGTH];

    uint8_t temp_bytecode_array[MAX_BYTECODE];
    struct Bytecode temp_bytecode = { 0, temp_bytecode_array };

    struct BObject *temp_memory_array[MAX_LINE_MEMORY];
    struct Memory temp_memory = { 0, temp_memory_array };

    struct Statement **program = calloc(MAX_PROG_SIZE, sizeof(*program));

    bool input_error = false;
    bool big_font = false;

    /* Fake statement storing data from INPUT command */
    struct Statement *input_stmt = NULL;

    while (true) {
        memset(input, 0, MAX_LINE_LENGTH);

        temp_bytecode.index = 0;
        memset(temp_bytecode_array, 0, MAX_BYTECODE);

        temp_memory.index = 0;
        memset(temp_memory_array, 0, sizeof(*temp_memory_array) * MAX_LINE_MEMORY);

        global_lineno = -1;

        if (file == stdin && !input_error) {
            printf("> ");
        }

        if (!fgets(input, MAX_LINE_LENGTH, file)) {
            if (file == stdin) {
                printf("\n");
                break;
            } else {
                fclose(file);
                file = stdin;
                if (run_file) {
                    strcpy(input, "RUN\n");
                }
            }
        }

        /* If user gives a line that exceeds length, ignore fgets input until we're parsed the
         * whole line */
        if (input[MAX_LINE_LENGTH - 2] != '\0') {
            if (!input_error) {
                runtime_error(NULL, "input line too long");
            }
            input_error = true;
            continue;
        } else if (input_error) {
            input_error = false;
            continue;
        }

        struct Statement *stmt = compile_line(input, &temp_memory, &temp_bytecode);
        if (!stmt) {
            /* Error */
            continue;
        } else if (stmt->lineno == 0) {
            /* No line number means we execute the command immediately */
            enum Status status = execute_line(stmt, vars, program, run_file,
                    &input_stmt, &filename, &big_font);

            /* Clear output parameters */
            run_file = false;
            if (input_stmt != NULL) {
                statement_free(input_stmt);
                input_stmt = NULL;
            }

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

