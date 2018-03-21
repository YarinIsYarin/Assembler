#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define MAX_LINE_SIZE  256
#define MAX_LABEL_SIZE 80
/* The maxs size of a line after being processed */
#define INNER_LINE_SIZE  MAX_LINE_SIZE*3
#define MAX_DATA_SIZE 256
#define MAX_FILE_NAME 20

#define TRUE 1
#define FALSE 0
#define ERROR -1

#define IMMEDIATE 0
#define VARIABLE 1
#define MATRIX 2
#define REGISTER 3
#define MAX_REGISTER 7

#define A 0
#define E 1
#define R 2

#define STARTING_ADDRESS 100

typedef struct sym_node * sym_ptr;
typedef struct sym_node {
    sym_ptr next;
    char * label;
    int address;

    /* Those are booleans 1 is true and 0 is false; */
    int ext, operation;
} symbol_table ;