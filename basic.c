// https://en.wikipedia.org/wiki/Tiny_BASIC
// http://www.ittybittycomputers.com/ittybitty/tinybasic/TBuserMan.txt
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>

enum TokenType {
    var,
    number,
    string,
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

struct List {
    void *elt;
    struct List *rest;
};

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
        char bvar;
        char *bstr;
    };
};

struct Statement {
    char *line;
    int len;
    char *bytecode;
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

// returns number of chars consumed
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
    printf("Error: unknown command '%s'\n", command);
    return 0;
}

int compile_command(char *input, int vars[26])
{
    // Get line number
    int lineno = 0;
    int chars_parsed = parse_lineno(input, &lineno);
    if (chars_parsed == -1) {
        return 0;
    }
    input += chars_parsed;
    while (isspace(*input)) input++;
    
    // Get command
    enum TokenType type;
    chars_parsed = parse_command(input, &type);
    if (!chars_parsed) {
        return 0;
    }
    input += chars_parsed;
    while (isspace(*input) || *input == '\0') input++;

    // Parse based on command
    switch (type) {
        case PRINT:
            printf("%s\n", input);
            break;
        default:
            printf("Command not implemented\n");
            break;
        case IF:
            break;
        case GOTO:
            break;
        case INPUT:
            break;
        case LET:
            break;
        case GOSUB:
            break;
        case RETURN:
            break;
        case CLEAR:
            break;
        case LIST:
            break;
        case RUN:
            break;
        case END:
            break;
        case REM:
            break;
        case DIM:
            break;
    }
    // If no line number given, execute immediately
    if (!lineno) {
        // run_command();
    }
    return 1;
}

#define MAX_INPUT 100
#define MAX_PROG_SIZE 10000

int main(void)
{
    printf("dan's basic interpreter\n");
    printf("press ctrl+c to exit\n");
    printf("type a command to continue\n");
    struct BObject vars[26] = {0};
    char input[MAX_INPUT];
    struct Statement **program = calloc(MAX_PROG_SIZE, sizeof(*program));
    while (1) {
        memset(input, 0, MAX_INPUT);
        printf("> ");
        if (!fgets(input, MAX_INPUT, stdin)) {
            printf("Error reading input\n");
        }
        compile_command(input, vars);
        // printf("%s", input);
    }
    printf("Hello world\n");
    free(program);
    return 0;
}

