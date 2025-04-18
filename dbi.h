#ifndef DBI_H
#define DBI_H

/* Hardcoded so max of 4 digit numbers for GOTOs and such - adjust as needed */
#define MAX_PROG_SIZE 10000

/* Hardcoded since variables can only be A-Z */
#define MAX_VARS 26

/* Should be set to whatever longest command name is (+1 character for null byte) */
#define MAX_COMMAND_NAME 7 

/* Arbitrary - adjust as needed */
#define MAX_LINE_LENGTH 256
#define MAX_STACK 128
#define MAX_CALL_STACK 128
#define MAX_LINE_MEMORY 64 /* NOTE: this should never be set to more than 256 
                                    since it will get used as a uint8_t */
#define MAX_BYTECODE 64

/* Toggling turns on some debug printing */
#define DEBUG 0

/* Feature flags */
#define FF_BIG 0
#define FF_SLEEP 0

int dbi_run_file(char *input_file_name);

#endif
