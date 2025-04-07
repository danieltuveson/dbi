// https://en.wikipedia.org/wiki/Tiny_BASIC
// http://www.ittybittycomputers.com/ittybitty/tinybasic/TBuserMan.txt
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>

// Hardcoded so max of 4 digit numbers for GOTOs and such
#define MAX_PROG_SIZE 10000

// Hardcoded since variables can be A-Z
#define MAX_VARS 26

// Arbitrary - adjust as needed
#define MAX_INPUT 100
#define MAX_STACK 100
#define MAX_LINE_MEM 16
#define MAX_LINE_PROGRAM 32

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
};

struct TokenMapping token_map[] = {
    { "PRINT",  PRINT },
    { "IF",     IF },
    { "GOTO",   GOTO },
    { "INPUT",  INPUT },
    { "LET",    LET },
    { "GOSUB",  GOSUB },
    { "RETURN", RETURN },
    { "CLEAR",  CLEAR },
    { "LIST",   LIST },
    { "RUN",    RUN },
    { "END",    END },
    { "REM",    REM },
    { "DIM",    DIM },
};

int token_map_size = sizeof(token_map) / sizeof(*token_map);

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
        char bvar;
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
            bobj_print(vars[(int)obj->bvar], vars);
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
    if (memory[0] != 0) {
        int size = sizeof(*stmt->memory) * mem_count;
        stmt->memory = malloc(size);
        memcpy(stmt->memory, memory, size);
    } else {
        stmt->memory = NULL;
    }

    // Bytecode
    stmt->byte_count = byte_count;
    if (bytecode[0] != 0) {
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
    OP_THEN,
    OP_GOTO,
    OP_INPUT,
    OP_LET,
    OP_GOSUB,
    OP_RETURN,
    OP_CLEAR,
    OP_LIST,
    OP_RUN,
    OP_END,
    OP_REM,
    OP_DIM,
    OP_LT,
    OP_GT,
    OP_EQ,
    OP_NEQ,
    OP_LEQ,
    OP_GEQ
};
// returns number of chars consumed
// -1 on error
int parse_lineno(char *input, int *lineno)
{
    *lineno = 0;
    int i = 0;
    while (isdigit(input[i])) {
        if (i > 4) {
            printf("Error: line number too large\n");
            return -1;
        }
        *lineno = *lineno * 10 + input[i] - '0';
        i++;
    }
    return i;
}

// Returns number of chars consumed
int parse_command(char *input, enum TokenType *type)
{
    if (input[0] == '\0') {
        printf("Error: empty line\n");
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

// Returns number of chars consumed
int compile_string(char *input, int mem_index, struct BObject **memory,
       int byte_index, uint8_t *bytecode)
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

    // Add string to statement memory
    memory[mem_index] = bstr_new(str_start, input - str_start);

    // Add location of string object to bytecode
    bytecode[byte_index] = OP_PUSH;
    byte_index++;
    bytecode[byte_index] = mem_index;

    return input - str_start + 2;
}

// TODO: Make this handle non-literal expressions
// Need to add error code for when this can allocate multiple slots in memory / multiple
// bytes in the bytecode array
int compile_expr(char *input, int *mem_index, struct BObject **memory, 
        int *byte_index, uint8_t *bytecode)
{
    char *str_start = input;

    int sign = 1;
    if (*input == '-') {
        sign = -1;
        input++;
    } else if (*input == '+') {
        input++;
    }
    int num = 0;
    while (isdigit(*input)) {
        num = 10 * num + *input - '0';
        input++;
    }

    // Add numeric literal to memory
    memory[*mem_index] = bint_new(sign * num);
    *mem_index = *mem_index + 1;

    // Add location of string object to bytecode
    bytecode[*byte_index] = OP_PUSH;
    *byte_index = *byte_index + 1;
    bytecode[*byte_index] = *mem_index - 1;
    *byte_index = *byte_index + 1;

    return input - str_start;
}

// Returns number of bytecodes added
// For now, only compile int / string literals
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
            goto cleanup;
        } else if (*mem_index >= MAX_LINE_MEM) {
            printf("Error: too many expressions\n");
            goto cleanup;
        }

        if (*input == '"') {
            char_count = compile_string(input, *mem_index, memory, byte_index, bytecode);
            if (char_count == 0) {
                goto cleanup;
            }
            *mem_index = *mem_index + 1;
            byte_index += 2;
        } else if (isdigit(*input) || *input == '+' || *input == '-' || *input == '(') {
            char_count = compile_expr(input, mem_index, memory, &byte_index, bytecode);
            if (char_count == 0) {
                goto cleanup;
            }
        } else if (*input == '\0') {
            printf("Error: unexpected end of print statement\n");
            goto cleanup;
        } else {
            printf("Error: invalid input: %s", input);
            goto cleanup;
        }

        bytecode[byte_index++] = OP_PRINT;

        input += char_count;

        while (isspace(*input)) input++;
        if (*input == ',') {
            input++;
            while (isspace(*input)) input++;
        } else {
            break;
        }
    } while (1);
    if (*input != '\0') {
        printf("Error: unexpected end of print statement\n");
        goto cleanup;
    }
    return byte_index;

cleanup:
    if (*mem_index > 0) {
        for (int i = 0; i < *mem_index; i++) {
            bobj_free(memory[i]);
        }
    }
    return 0;
}

// Returns number of bytes in bytecode
struct Statement *compile_command(char *input, struct BObject **temp_memory, uint8_t *bytecode)
{
    char *init_input = input;

    // Get line number
    int lineno = 0;
    int chars_parsed = parse_lineno(input, &lineno);
    if (chars_parsed == -1) {
        return NULL;
    }
    input += chars_parsed;
    while (isspace(*input)) input++;
    
    // Get command
    enum TokenType type;
    chars_parsed = parse_command(input, &type);
    if (!chars_parsed) {
        return NULL;
    }
    input += chars_parsed;
    while (isspace(*input)) input++;

    // Parse based on command
    int byte_count = 0;
    int mem_count = 0;
    switch (type) {
        case PRINT:
            byte_count = compile_expr_list(input, &mem_count, temp_memory, bytecode);
            if (byte_count == 0) {
                return NULL;
            }
            break;
        case IF:
            break;
        case INPUT:
            break;
        case LET:
            break;
        case GOTO:
            break;
        case GOSUB:
            break;
        case RETURN:
            byte_count = 1;
            bytecode[0] = OP_RETURN;
            break;
        case CLEAR:
            byte_count = 1;
            bytecode[0] = OP_CLEAR;
            break;
        case LIST:
            byte_count = 1;
            bytecode[0] = OP_LIST;
            break;
        case RUN:
            byte_count = 1;
            bytecode[0] = OP_RUN;
            break;
        case END:
            byte_count = 1;
            bytecode[0] = OP_END;
            break;
        case DIM:
            break;
        case REM:
            byte_count = 1;
            bytecode[0] = OP_NO;
            break;
        default:
            printf("Command not implemented\n");
            break;
    }
    return statement_new(lineno, init_input, mem_count, temp_memory, byte_count, bytecode);
}

// *******************************************************************
// ************************* VM / Execution ************************** 
// *******************************************************************

enum Status {
    STATUS_FINISHED,
    STATUS_GOOD,
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

enum Status execute_statement(struct Statement *stmt, struct BObject **vars,
        struct Statement **program);

enum Status program_run(struct Statement **program, struct BObject **vars)
{
    enum Status status = STATUS_GOOD;
    for (int i = 0; i < MAX_PROG_SIZE; i++) {
        struct Statement *stmt = program[i];
        if (!stmt) {
            continue;
        }
        status = execute_statement(stmt, vars, program);
        if (status == STATUS_FINISHED || status == STATUS_ERROR) {
            break;
        }
    }
    return status;
}

#define push(stack, val)\
    memcpy(&(stack[++stack_offset]), val, sizeof(*val))

#define pop(stack)\
    &(stack[stack_offset--])

enum Status execute_statement(struct Statement *stmt, struct BObject **vars,
        struct Statement **program)
{
    enum Status status = STATUS_GOOD;
    static int stack_offset = 0;
    static struct BObject stack[MAX_STACK];
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
#endif
    int mem_loc, has_print;
    has_print = 0;
    for (int i = 0; i < stmt->byte_count; i++) {
        uint8_t op = stmt->bytecode[i];
        switch (op) {
            case OP_NO:
                break;
            case OP_PUSH:
                mem_loc = stmt->bytecode[++i];
                push(stack, stmt->memory[mem_loc]);
                break;
            case OP_PRINT:
                struct BObject *obj = pop(stack);
                bobj_print(obj, vars);
                has_print = 1;
                break;
            case OP_IF:
                break;
            case OP_INPUT:
                break;
            case OP_LET:
                break;
            case OP_GOTO:
                break;
            case OP_GOSUB:
                break;
            case OP_RETURN:
                break;
            case OP_CLEAR:
                program_clear(program);
                break;
            case OP_LIST:
                program_list(program);
                break;
            case OP_RUN:
                status = program_run(program, vars);
                break;
            case OP_END:
                return STATUS_FINISHED;
            case OP_DIM:
                break;
            default:
                printf("Internal error: unknown command encountered\n");
        }
    }
    if (has_print) {
        printf("\n");
    }
    return status;
}

int main(void)
{
    printf("dan's basic interpreter\n");
    printf("press ctrl+d or type 'end' to exit\n");
    printf("type a command to continue\n");

    struct BObject *vars[MAX_VARS] = {0};
    for (int i = 0; i < MAX_VARS; i++) {
        vars[i] = malloc(sizeof(struct BObject));
        vars[i]->type = BOB_INT;
        vars[i]->bint = 0;
    }

    char input[MAX_INPUT];
    struct BObject *temp_memory[MAX_LINE_MEM];
    uint8_t temp_bytecode[MAX_LINE_PROGRAM];

    struct Statement **program = calloc(MAX_PROG_SIZE, sizeof(*program));

    while (1) {
        memset(input, 0, MAX_INPUT);
        memset(temp_memory, 0, sizeof(*temp_memory) * MAX_LINE_MEM);
        memset(temp_bytecode, 0, MAX_LINE_PROGRAM);

        printf("> ");
        if (!fgets(input, MAX_INPUT, stdin)) {
            printf("Error reading input\n");
            break;
        } else if (feof(stdin)) {
            printf("\n");
            break;
        }

        struct Statement *stmt = compile_command(input, temp_memory, temp_bytecode);
        if (!stmt) {
            // Error
            continue;
        } else if (stmt->lineno == 0) {
            // No line number means we execute the command immediately
            enum Status status = execute_statement(stmt, vars, program);
            statement_free(stmt);
            if (status == STATUS_FINISHED) {
                break;
            }
        } else {
            program[stmt->lineno] = stmt;
        }
    }

    for (int i = 0; i < MAX_VARS; i++) {
        free(vars[i]);
    }
    program_clear(program);
    free(program);
    return 0;
}

