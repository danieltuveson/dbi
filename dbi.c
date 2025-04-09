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

// Hardcoded so max of 4 digit numbers for GOTOs and such
#define MAX_PROG_SIZE 10000

// Hardcoded since variables can be A-Z
#define MAX_VARS 26

// Arbitrary - adjust as needed
#define MAX_INPUT 200
#define MAX_STACK 100
#define MAX_LINE_MEM 16
#define MAX_LINE_PROGRAM 32
#define MAX_SUB_DEPTH 32

// Toggling turns on some debug printing
#define DEBUG 0

// *******************************************************************
// ***************************** Tokens ******************************
// *******************************************************************

enum TokenType {
    VAR,
    NUMBER,
    STRING,
    // Simple tokens
    PRINT,
    IF,
    THEN,
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
    DIM,
    LOAD,
    SAVE,
    HELP,
    QUOTE,
    // Operators
    LT,
    GT,
    EQ,
    NEQ,
    LEQ,
    GEQ
};

struct TokenMapping {
    char *str;
    enum TokenType type;
    char *helpstr;
    char *helpex;
};

struct TokenMapping token_map[] = {
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
    // { "DIM",    DIM,    "creates array",                                      "DIM (TBD)" },
    { "HELP",   HELP,   "you just ran this",                                  "HELP" },
    { "QUOTE",  QUOTE,  "???",                                                "QUOTE" },
};

int token_map_size = sizeof(token_map) / sizeof(*token_map);

struct GrammarMapping {
    char *symbol;
    char *expression;
};

struct GrammarMapping grammar_map[] = {
    { "expr-list", "(string|expr) (, (string|expr) )*" },
    { "var-list", "var (, var)*" },
    { "expr", "(+|-|ε) term ((+|-) term)*" },
    { "term", "factor ((*|/) factor)*" },
    { "factor", "var | number | (expr)" },
    { "var", "A | B | C ... | Y | Z" },
    { "number", "digit digit*" },
    { "digit", "0 | 1 | 2 | 3 | ... | 8 | 9" },
    { "relop", "< (>|=|ε) | > (<|=|ε) | =" },
    { "string", "\" (non-quote-characters)* \"" },
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
    for (int i = 0; i < token_map_size; i++) {
        struct TokenMapping tm = token_map[i];
        printf(" %-8s|  %-52s|  %-25s\n", tm.str, tm.helpstr, tm.helpex);
    }
    print_line();
    for (int i = 0; i < grammar_map_size; i++) {
        struct GrammarMapping gm = grammar_map[i];
        printf("%-9s  ::=  %s\n", gm.symbol, gm.expression);
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
"“It is practically impossible to teach good programming to students\n"
"that have had a prior exposure to BASIC: as potential programmers\n"
"they are mentally mutilated beyond hope of regeneration.”\n"
"― Edsger Dijkstra";

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

void bobj_print(struct BObject *obj, struct BObject **vars)
{
    switch (obj->type) {
        case BOB_INT:
            printf("%d", obj->bint);
            break;
        case BOB_STR:
            printf("%s", obj->bstr);
            break;
        case BOB_VAR:
            bobj_print(vars[obj->bvar], vars);
            break;
    }
}

// *******************************************************************
// ************************* Basic Statement ************************* 
// *******************************************************************

struct Statement {
    int lineno;
    char *line;
    // list of BObjects used by statement
    int mem_count;
    struct BObject **memory; 
    int byte_count;
    uint8_t *bytecode;
};

struct Statement *statement_new(int lineno, char *input, int mem_count, struct BObject **memory,
        int byte_count, uint8_t *bytecode)
{
    struct Statement *stmt = malloc(sizeof(*stmt));

    // Line info
    stmt->lineno = lineno;
    stmt->line = malloc(strlen(input) + 1);
    strcpy(stmt->line, input);

    // Memory
    stmt->mem_count = mem_count;
    if (mem_count != 0) {
        int size = sizeof(*stmt->memory) * mem_count;
        stmt->memory = malloc(size);
        memcpy(stmt->memory, memory, size);
    } else {
        stmt->memory = NULL;
    }

    // Bytecode
    stmt->byte_count = byte_count;
    if (byte_count != 0) {
        stmt->bytecode = malloc(byte_count);
        memcpy(stmt->bytecode, bytecode, byte_count);
    } else {
        stmt->bytecode = NULL;
    }
    return stmt;
}

void statement_free(struct Statement *stmt)
{
    free(stmt->line);
    if (stmt->memory) {
        for (int i = 0; i < stmt->mem_count; i++) {
            bobj_free(stmt->memory[i]);
        }
        free(stmt->memory);
    }
    if (stmt->bytecode) {
        free(stmt->bytecode);
    }
    free(stmt);
}

// *******************************************************************
// *********************** Parsing / Compiling *********************** 
// *******************************************************************

enum Opcode {
    OP_NO, // no-op
    OP_PUSH,
    OP_PRINT,
    OP_IF,
    OP_GOTO,
    OP_SUBPUSH,
    OP_INPUT,
    OP_LET,
    OP_RETURN,
    OP_CLEAR,
    OP_LIST,
    OP_RUN,
    OP_END,
    OP_LOAD,
    OP_SAVE,
    OP_DIM,
    OP_HELP,
    OP_LT,
    OP_GT,
    OP_EQ,
    OP_NEQ,
    OP_LEQ,
    OP_GEQ
};

void ignore_whitespace(char **input_ptr)
{
    char *input = *input_ptr;
    while (isspace(*input)) input++;
    *input_ptr = input;
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
            printf("Error: line number exceeds maximum value of %d\n", MAX_PROG_SIZE - 1);
            return -1;
        }
        *lineno = *lineno * 10 + input[i] - '0';
        i++;
    }
    if (*lineno == 0) {
        printf("Error: line number cannot be 0\n");
        return -1;
    }
    return i;
}

// Returns number of chars consumed
int parse_command_name(char *input, enum TokenType *type)
{
    if (input[0] == '\0') {
        return 0;
    }
    char command[7] = {0};
    int i = 0;
    while (!isspace(input[i]) && input[i] != '\0') {
        if (i >= 7) {
            goto err;
        }
        command[i] = input[i];
        if ('a' <= input[i] && input[i] <= 'z') {
            command[i] -= 0x20;
        }
        i++;
    }
    for (int j = 0; j < token_map_size; j++) {
        int len = strlen(token_map[j].str);
        if (i == len && strncmp(command, token_map[j].str, len) == 0) {
            *type = token_map[j].type;
            return i;
        }
    }
err:
    printf("Error: unknown command\n");
    return 0;
}

// Helper function to clear partially initialized memory on compile error
void mem_clear(struct BObject **memory, int index)
{
    if (index > 0) {
        for (int i = 0; i < index; i++) {
            bobj_free(memory[i]);
            memory[i] = NULL;
        }
    }
}

// Returns number of chars consumed
int compile_string(char *input, int *mem_index, struct BObject **memory,
        int *byte_index, uint8_t *bytecode)
{
    input++; // discard opening quote
    char *str_start = input;
    while (*input != '"') {
        if (*input == '\0') {
            printf("Error: unexpected end of string\n");
            return 0;
        }
        input++;
    }

    // Add location of string object to bytecode
    bytecode[*byte_index] = OP_PUSH;
    *byte_index = *byte_index + 1;
    bytecode[*byte_index] = *mem_index;
    *byte_index = *byte_index + 1;

    // Add string to statement memory
    memory[*mem_index] = bstr_new(str_start, input - str_start);
    *mem_index = *mem_index + 1;

    return input - str_start + 2;
}

#define push_op(op)\
    stack[++op_stack_offset]

#define pop_op()\
    stack[op_stack_offset--]

// TODO: Make this handle non-literal expressions
// Need to add error code for when this can allocate multiple slots in memory / multiple
// bytes in the bytecode array
int compile_expr(char *input, int *mem_index, struct BObject **memory, 
        int *byte_index, uint8_t *bytecode)
{
    char *str_start = input;

    // int sign = 1;
    // if (*input == '-') {
    //     sign = -1;
    //     input++;
    // } else if (*input == '+') {
    //     input++;
    // }

    // Shunting yard
    char op_stack[MAX_STACK];
    int op_stack_offset = 0;
    while (*input != '\0') {
        ignore_whitespace(&input);

        if (isdigit(*input)) {
            int num = 0;
            while (isdigit(*input)) {
                num = 10 * num + *input - '0';
                input++;
            }

            // Add numeric literal to memory
            memory[*mem_index] = bint_new(num);
            *mem_index = *mem_index + 1;

        } else if (('a' <= *input && *input <= 'z') || ('A' <= *input && *input <= 'Z')) {
            
            uint8_t var = *input >= 'a' ? *input - 'a' : *input - 'A';
            input++;

            // Add variable to memory
            memory[*mem_index] = bvar_new(var);
            *mem_index = *mem_index + 1;

        // } else {
        //     printf("Error: expected number or variable\n");
        //     mem_clear(memory, *mem_index);
        //     return 0;
        } else {
            break;
        }

        // ignore_whitespace(&input);
        // if (*input != '\0') {
        //     printf("Error: unexpected characters in expression\n");
        //     mem_clear(memory, *mem_index);
        //     return 0;
        // }


        // Add location of object to bytecode
        bytecode[*byte_index] = OP_PUSH;
        *byte_index = *byte_index + 1;
        bytecode[*byte_index] = *mem_index - 1;
        *byte_index = *byte_index + 1;
    }

    return input - str_start;
}

// 1 if success, 0 if fail
int compile_solo_expr(char *input, int *mem_index, struct BObject **memory,
        int *byte_index, uint8_t *bytecode)
{
    ignore_whitespace(&input);
    int char_count = compile_expr(input, mem_index, memory, byte_index, bytecode);
    if (char_count == 0) {
        printf("Error: unexpected characters in expression\n");
        mem_clear(memory, *mem_index);
        return 0;
    }
    input += char_count;
    ignore_whitespace(&input);
    if (*input != '\0') {
        printf("Error: unexpected end of statement\n");
        mem_clear(memory, *mem_index);
        return 0;
    }
    return 1;
}

// Returns number of bytecodes added
// For now, only compile int / string literals
// TODO: Make this more consistent with the other "compile_" functions
int compile_expr_list(char *input, int *mem_index, struct BObject **memory, uint8_t *bytecode)
{
    *mem_index = 0;
    int byte_index = 0;
    int char_count = 0;

    do {
        // Need to reconfigure this error checking a bit
        // Maybe doesn't even matter since line length is restricted...
        if (byte_index >= MAX_LINE_PROGRAM) {
            printf("Error: expression too long\n");
            mem_clear(memory, *mem_index);
            return 0;
        } else if (*mem_index >= MAX_LINE_MEM) {
            printf("Error: too many expressions\n");
            mem_clear(memory, *mem_index);
            return 0;
        }

        if (*input == '"') {
            char_count = compile_string(input, mem_index, memory, &byte_index, bytecode);
            if (char_count == 0) {
                mem_clear(memory, *mem_index);
                return 0;
            }
        } else if (('a' <= *input && *input <= 'z') || ('A' <= *input && *input <= 'Z') ||
                isdigit(*input) || *input == '+' || *input == '-' || *input == '(') {
            char_count = compile_expr(input, mem_index, memory, &byte_index, bytecode);
            if (char_count == 0) {
                // compile_expr will clear memory on error
                return 0;
            }
        } else if (*input == '\0') {
            printf("Error: unexpected end of statement\n");
            mem_clear(memory, *mem_index);
            return 0;
        } else {
            printf("Error: invalid input: %s", input);
            mem_clear(memory, *mem_index);
            return 0;
        }

        bytecode[byte_index++] = OP_PRINT;

        input += char_count;

        ignore_whitespace(&input);
        if (*input == ',') {
            input++;
            ignore_whitespace(&input);
        } else {
            break;
        }
    } while (1);
    if (*input != '\0') {
        printf("Error: unexpected end of statement\n");
        mem_clear(memory, *mem_index);
        return 0;
    }
    return byte_index;
}

int compile_let(char *input, int *mem_index, struct BObject **memory,
        int *byte_index, uint8_t *bytecode)
{
    if (!(('a' <= *input && *input <= 'z') || ('A' <= *input && *input <= 'Z'))) {
        printf("Error: expected variable name\n");
        return 0;
    }
    uint8_t var = *input >= 'a' ? *input - 'a' : *input - 'A';
    input++;

    ignore_whitespace(&input);
    if (*input != '=') {
        printf("Error: missing '=' in LET statement\n");
        return 0;
    }
    input++;

    ignore_whitespace(&input);
    if (!compile_solo_expr(input, mem_index, memory, byte_index, bytecode)) {
        return 0;
    }
    if (*byte_index + 1 >= MAX_LINE_PROGRAM) {
        printf("Error: expression too long\n");
        mem_clear(memory, *mem_index);
        return 0;
    }
    bytecode[*byte_index] = OP_LET;
    *byte_index = *byte_index + 1;
    bytecode[*byte_index] = var;
    *byte_index = *byte_index + 1;
    return 1;
}

// Returns number of bytes in bytecode
struct Statement *compile_command(char *input, struct BObject **temp_memory, uint8_t *bytecode)
{
    char *init_input = input;
    ignore_whitespace(&input);

    // Get line number
    int lineno = 0;
    int chars_parsed = parse_lineno(input, &lineno);
    if (chars_parsed == -1) {
        return NULL;
    }
    input += chars_parsed;
    ignore_whitespace(&input);

    // Get command
    enum TokenType type;
    chars_parsed = parse_command_name(input, &type);
    if (!chars_parsed) {
        return NULL;
    }
    input += chars_parsed;
    ignore_whitespace(&input);

    // Parse based on command
    int byte_index = 0;
    int mem_index = 0;
    switch (type) {
        case PRINT:
            byte_index = compile_expr_list(input, &mem_index, temp_memory, bytecode);
            if (byte_index == 0) {
                return NULL;
            }
            break;
        case IF:
            break;
        case INPUT:
            break;
        case LET:
            compile_let(input, &mem_index, temp_memory, &byte_index, bytecode);
            if (byte_index == 0) {
                return NULL;
            }
            break;
        case GOSUB:
            bytecode[byte_index++] = OP_SUBPUSH;
            bytecode[byte_index++] = lineno + 1;
            /* fall through */
        case GOTO:
            if (!compile_solo_expr(input, &mem_index, temp_memory, &byte_index, bytecode)) {
                return NULL;
            }
            if (byte_index >= MAX_LINE_PROGRAM) {
                printf("Error: expression too long\n");
                mem_clear(temp_memory, mem_index);
                return NULL;
            } else {
                bytecode[byte_index++] = OP_GOTO;
            }
            break;
        case RETURN:
            bytecode[byte_index++] = OP_RETURN;
            break;
        case CLEAR:
            bytecode[byte_index++] = OP_CLEAR;
            break;
        case LIST:
            bytecode[byte_index++] = OP_LIST;
            break;
        case RUN:
            bytecode[byte_index++] = OP_RUN;
            break;
        case END:
            bytecode[byte_index++] = OP_END;
            break;
        // case DIM:
        //     break;
        case REM:
            bytecode[byte_index++] = OP_NO;
            break;
        case LOAD:
        case SAVE:
            if (*input != '"') {
                printf("Error: expected file name\n");
                return NULL;
            }
            chars_parsed = compile_string(input, &mem_index, temp_memory, &byte_index, bytecode);
            if (chars_parsed == 0) {
                return NULL;
            }
            byte_index = 2;
            input += chars_parsed;
            bytecode[byte_index++] = type == LOAD ? OP_LOAD : OP_SAVE;
            break;
        case QUOTE:
            bytecode[byte_index++] = OP_PUSH;
            bytecode[byte_index++] = 0;
            bytecode[byte_index++] = OP_PRINT;
            temp_memory[mem_index++] = bstr_new(quote, strlen(quote));
            break;
        case HELP:
            bytecode[byte_index++] = OP_HELP;
            break;
        default:
            printf("Command not implemented\n");
            return NULL;
    }
    // TODO: clean up above functions so that the following doesn't throw errors
    // ignore_whitespace(&input);
    // if (*input != '\0') {
    //     printf("Error: unexpected input '%c'\n", *input);
    //     mem_clear(temp_memory, mem_index);
    //     return NULL;
    // }
    return statement_new(lineno, init_input, mem_index, temp_memory, byte_index, bytecode);
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
    if (stmt->lineno == 0) {
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

#define pop()\
    &(stack[stack_offset--])

#define push_sub(line)\
    substack[++substack_offset] = line

#define pop_sub()\
    substack[substack_offset--]

enum Status execute_statement(
        struct Statement *stmt,
        struct BObject **vars,
        struct Statement **program,
        char **filename)
{
    enum Status status = STATUS_GOOD;

    int stack_offset = 0;
    struct BObject stack[MAX_STACK];

    int substack_offset = 0;
    int substack[MAX_STACK];

    struct BObject *obj;
    int mem_loc, line;
    int has_print = 0;
    int ip = 0;
    while (1) {
        uint8_t op = stmt->bytecode[ip];
#if DEBUG
    printf("stmt->byte_count:%d\n", stmt->byte_count);
    printf("stmt->mem_count:%d\n", stmt->mem_count);
    printf("mem: %p\n", stmt->memory);
    printf("bytecode {");
    for (int i = 0; i < stmt->byte_count; i++) {
        if (i != 0) printf(", ");
        printf("%d", stmt->bytecode[i]);
    }
    printf("}\n");
    printf("op: %d\n", op);
#endif

        switch (op) {
            case OP_NO:
                break;
            case OP_PUSH:
                mem_loc = stmt->bytecode[++ip];
                if (stack_offset + 1 >= MAX_STACK) {
                    runtime_error(stmt, "stack overflow");
                    return STATUS_ERROR;
                }
                push(stmt->memory[mem_loc]);
                break;
            case OP_PRINT:
                obj = pop();
                bobj_print(obj, vars);
                has_print = 1;
                break;
            case OP_IF:
                break;
            case OP_INPUT:
                break;
            case OP_LET:
                obj = pop();
                mem_loc = stmt->bytecode[++ip];
                if (obj->type == BOB_VAR) {
                    if (mem_loc != obj->bvar) {
                        memcpy(vars[mem_loc], vars[obj->bvar], sizeof(*vars[mem_loc]));
                    }
                } else {
                    memcpy(vars[mem_loc], obj, sizeof(*vars[mem_loc]));
                }
                break;
            case OP_GOTO:
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
            case OP_SUBPUSH:
                line = stmt->bytecode[++ip];
                push_sub(line);
                break;
            case OP_RETURN:
                if (substack_offset <= 0) {
                    runtime_error(stmt, "no outer subroutine to return to");
                    return STATUS_ERROR;
                }
                line = pop_sub();
                stmt = statement_next(program, line);
                if (!stmt) {
                    runtime_error(stmt, "could not return from subroutine");
                    return STATUS_ERROR;
                }
                ip = 0;
                continue;
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
            case OP_DIM:
                break;
            case OP_HELP:
                print_help();
                break;
            default:
                printf("Internal error: unknown command encountered\n");
                return STATUS_ERROR;
        }
        ip++;
        if (ip >= stmt->byte_count) {
            if (has_print) {
                has_print = 0;
                printf("\n");
            }
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
        vars[i] = malloc(sizeof(struct BObject));
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
    struct BObject *temp_memory[MAX_LINE_MEM];
    uint8_t temp_bytecode[MAX_LINE_PROGRAM];

    struct Statement **program = calloc(MAX_PROG_SIZE, sizeof(*program));

    FILE *file = stdin;
    char *filename;

    while (1) {
        memset(input, 0, MAX_INPUT);
        memset(temp_memory, 0, sizeof(*temp_memory) * MAX_LINE_MEM);
        memset(temp_bytecode, 0, MAX_LINE_PROGRAM);

        if (file == stdin) printf("> ");
        if (!fgets(input, MAX_INPUT, file)) {
            if (file == stdin) {
                printf("\n");
                break;
            } else {
                fclose(file);
                file = stdin;
            }
        }

        struct Statement *stmt = compile_command(input, temp_memory, temp_bytecode);
        if (!stmt) {
            // Error
            continue;
        } else if (stmt->lineno == 0) {
            // No line number means we execute the command immediately
            enum Status status = execute_statement(stmt, vars, program, &filename);
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

