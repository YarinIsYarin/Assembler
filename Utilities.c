#include "mmn14.h"
#include <ctype.h>

void addToSym (sym_ptr * head, char * label, int address, int ext, int operation);
int howManyOperands (char *instruction);
int operandType (char * operands);
void deleteWords (int i, char *line);
int readNum (int *out, char * read);
sym_ptr findSym (char * name, sym_ptr head);
void writeBase4 (int num, int digits, FILE * fp, int compact);
int readMatrix (char *firstParam, char *secondParam, char * matrix);
int writeAddress (FILE * out);

/* Receives the head of a list and adds a new node to the end of the given list*/
void addToSym (sym_ptr * head, char * label, int address, int ext, int operation) {
    sym_ptr p, veryNew = (sym_ptr) malloc(sizeof(symbol_table));
    if (!veryNew) {
        printf("Error: memory allocation failed");
        return;
    }
    if (!veryNew) {
        printf("Error: failed memory allocation\n");
        return;
    }
    veryNew -> next = NULL;
    veryNew -> label = malloc(sizeof(label));
    strcpy(veryNew -> label, label);
    veryNew -> address = address;
    veryNew -> ext = ext;
    veryNew -> operation = operation;

    if (*head == NULL) {
        *head = veryNew;
    } else {
        p = *head;
        while (p -> next != NULL)
            p = p -> next;
        p -> next = veryNew;
    }
}

/* Receives an instruction and returns how many operands the command should receive*/
int howManyOperands (char *instruction) {
    char  * zeroOperands[] = {"rts", "stop", "$"};
    char  * oneOperands[] = {"not", "clr", "inc", "dec", "jmp", "bne", "red", "prn", "jsr", "$"};
    char  * twoOperands[] = {"mov", "cmp", "add", "sub", "lea", "$"};

    for (int i = 0; strcmp(zeroOperands[i],"$") != 0; i++) {
        if (strcmp(zeroOperands[i], instruction) == 0)
            return 0;
    }

    for (int i = 0; strcmp(oneOperands[i],"$") != 0; i++)
        if (strcmp(oneOperands[i], instruction) == 0)
            return 1;

    for (int i = 0; strcmp(twoOperands[i],"$") != 0; i++)
        if (strcmp(twoOperands[i], instruction) == 0)
            return 2;
    return ERROR; /* returns error if the given instruction doesn't exists */
}

/* Receives an operand and returns its type */
int operandType (char * operands) {
    int i;
    if (!operands)
        return ERROR;
    if (operands[0] == '#')
        return IMMEDIATE;
    if (operands[0] == 'r' && operands[1] >= '0' &&
            operands[1] <= '0' + MAX_REGISTER && operands[2] == '\0')
        return REGISTER;
    /* If it contains the char [ then it must a matrix */
    for (i = 0; operands[i] !='\0'; i++)
        if (operands[i] == '[')
            return MATRIX;
    /* if reached where the operand is a variable or an error, we will detect those
     kind of errors later */
    return VARIABLE;
}

/* Removes the first i words from a line (read by readLine),
   the line must have at least i words */
void deleteWords (int i, char *line) {
    char temp[INNER_LINE_SIZE];
    int j;

    /* Finds the end of the i-th word */
    for (j = 0; i > 0; j++)
        if (line[j] == ' ')
            i--;
    /* copy after the i-th until end of the string to temp */
    for (i = 0; line[i+j] != '\0'; i++)
        temp[i] = line[j+i];
    temp[i] = '\0';
    strcpy(line, temp);
}

/* Converts a string to a integer, if the string is not a int returns ERROR, otherwise returns TRUE*/
int readNum (int *out, char * read) {
    int i;
    if (!read)
        return ERROR;
    /* Make sure all of the string is a number (atoi doesn't do that) */
    for (i = 1; read[i] != '\0'; i++)
        if (!isdigit(read[i]))
            return ERROR;

    *out = atoi(read);
    if (*out)
        return TRUE;
    /* if atoi returned 0 it could be zero or an illegal int */
    if (read[0] != '0' && read[0] != '-')
        return ERROR;
    for (i = 1; read[i] != '\0'; i++)
        if (read[i] != 0)
            return ERROR;
    return TRUE;
}

/* This func receives a name and the symbol table and check if the there is a symbol with
   the given name, if there is it returns a pointer to it, else returns NULL */
sym_ptr findSym (char * name, sym_ptr head) {
    /* Go over every node in the list*/
    while (head != NULL) {
        if (strcmp(name, head -> label) == 0) {
            return head;}
        head = head -> next;
    }
    return NULL;
}

/* Writes a given number to a given file in weird base 4 (0=a, 1=b, 2=c, 4=d),
   if compact if false it will print the number using a given amount of digits*/
void writeBase4 (int num, int digits, FILE * fp, int compact) {
    int base2 [digits * 2];
    char  out[digits];
    char *ptr = out;
    int i = 0, sign = 1;
    if (num < 0) {
        num = -num;
        sign = -1;
    }
    /* convert to base 2*/
    while (num > 0) {
        base2[i++] = num % 2;
        num /= 2;
    }
    i--;
    /* fill the rest of the array with zeros*/
    for (i++; i < digits*2; i++) {
        base2[i] = 0;
    }

    /* Do two's complement */
    if (sign == -1) {
        int carry = 1;
        for (i = 0; i < digits*2; i++)
            if (base2[i] == 1) {
                base2[i] = 0;
            } else {
                base2[i] = 1;
            }
        for (i = 0; i < digits*2; i++)
            if (carry == 1) {
                if (base2[i] == 1) {
                    base2[i] = 0;
                } else {
                    base2[i] = 1;
                    carry = 0;
                }
            }
    }

    /* save the result in weird base 4 */
    for (i = 0; i < digits; i++) {
        int temp = 2*base2[2*i + 1] +  base2[2*i];
        out[digits - 1 - i] = temp + 'a';
    }
    out[i] = '\0';
    /* If compact flag is true then we need to write it with as few digits as possible
     * so we will find were the unnecessary 'a's are and not print them */
    if (compact)
        while (ptr[0] == 'a' && ptr[1] != '\0')
            ptr++;

    fputs(ptr,fp);
}

/* Receives a matrix (without white chars) and stores the values between the brackets*/
int readMatrix (char *firstParam, char *secondParam, char * matrix) {
    int i,j;
    /* Find the first [ */
    for (i = 0; matrix[i] != '['; i++)
        if (matrix[i] == '\0') {
            printf("Error: Invalid matrix size");
            return ERROR;
        }
    i++;
    /* Store the first param of the matrix */
    for (j = 0; matrix[i+j] != ']'; j++) {
        if (matrix[i+j] == '\0') {
            printf("Error: Invalid matrix size");
            return ERROR;
        }
        firstParam[j] = matrix[i+j];
    }
    firstParam[j] = '\0';

    /* Find the second [ */
    for (i = j+i; matrix[i] != '['; i++)
        if (matrix[i] == '\0') {
            printf("Error: Invalid matrix size");
            return ERROR;
        }
    i++;
    /* Store the second param of the matrix */
    for (j = 0; matrix[i+j] != ']'; j++) {
        if (matrix[i+j] == '\0') {
            printf("Error: Invalid matrix size");
            return ERROR;
        }
        secondParam[j] = matrix[i+j];
    }
    /* Make sure the matrix ends */
    if (matrix[j+i+1] != '\0') {
        printf("Error: Additional matrix data");
        return ERROR;
    }
    secondParam[j] = '\0';
    return TRUE;
}

/* Prints the current address and updates it (keeps it as a static variable) */
int writeAddress (FILE * out) {
    static int address = STARTING_ADDRESS;
    /* Address is always the start of a new line */
    fputc('\n',out);
    writeBase4(address,4,out,FALSE);
    fputc('\t',out);
    address++;
    return address-1;
}