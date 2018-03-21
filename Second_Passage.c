#include "mmn14.h"

sym_ptr findSym (char * name, sym_ptr head);
void writeBase4 (int num, int digits, FILE * fp, int compact);
int readMatrix (char *firstParam, char *secondParam, char * matrix);
int writeAddress (FILE * out);
int howManyOperands (char *instruction);
int operandType (char * operands);
int readNum (int *out, char * read);
void deleteWords (int i, char *line);

int isEntry (char line[], sym_ptr head, FILE * ent);
int incode (char *line, FILE * output, FILE *ext, sym_ptr head);
int incodeOperand (char * operand, FILE * output, FILE * ext, sym_ptr head);
void writeReg (char * source, char * dest, FILE * fp);

/* Deals with .entry, returns TRUE if it did, FALSE if there wasn't
   an extern in that line and ERROR if the line is an extern but invalid */
int isEntry (char line[], sym_ptr head, FILE * ent) {
    char temp[INNER_LINE_SIZE];
    char *word; /* Points at the current word in the line we are processing */
    sym_ptr p;

    strcpy(temp,line);
    word = strtok(temp, " ");
    if (!word)
        return FALSE;
    if (strcmp(word,".entry") == 0) {
        word = strtok(NULL, " ");
        if (!word) {
            printf("Error: missing name at line ");
            return ERROR;
        }
        if (!(p = findSym(word, head))) {
            printf("Error: unknown identifier \"%s\" at line ", word);
            return ERROR;
        }
        if (p->ext) {
            printf("Error: external value \"%s\" cant be saved to entry at line ", word);
            return ERROR;
        }
        fputs(word, ent);
        fputc('\t', ent);
        writeBase4(p->address, 4, ent, FALSE);
        fputc('\n', ent);
    }
    return TRUE;
}

/* Receives a line (read by readLine) and the first word in the line MUST be and
 * instruction, incode writes the command in weird base 4 to output. returns ERROR if the
 * line has an error, TRUE otherwise*/
int incode (char *line, FILE * output, FILE *ext, sym_ptr head) {
    char *instructions[] = {"mov", "cmp", "add", "sub", "not", "clr", "lea", "inc",
                            "dec", "jmp", "bne", "red", "prn", "sjr", "rts", "stop",};
    /* the opcode of the instruction in i-th index is i*/
    int firstOperandType = 0, secondOperandType, i, operandsCount;
    char *word, /*points to the start of the word we are currently processing */
            *firstOperand = NULL, *secondOperand = NULL/* Used to save the location of the operand in the line*/;
    char temp[INNER_LINE_SIZE];
    strcpy(temp, line);
    word = strtok(temp, " ");
    operandsCount = howManyOperands(word);
    for (i = 0; strcmp(word, instructions[i]); i++);
    writeAddress(output);
    writeBase4(i, 2, output,FALSE);

    word = strtok(NULL, " ");
    if (operandsCount == 0) {
        if (word) {
            printf("Error: Additional data");
            return ERROR;
        }
        writeBase4(0,2,output,FALSE);
        writeBase4(A,1,output,FALSE);
        return TRUE;
    }
    if (operandsCount == 2) {
        firstOperandType = operandType(word);
        if (firstOperandType == ERROR) {
            printf("Error: Missing data about source operand");
            return ERROR;
        }
        firstOperand = word; /* save the place of the operand for later */

        word = strtok(NULL, " ");
        if (!word) {
            printf("Error: missing comma");
            return ERROR;
        }
        if (strcmp(word, ",") != 0) {
            printf("Error: Missing comma");
            return ERROR;
        }
        word = strtok(NULL, " ");
    }
    if (!word) {
        printf("Error: Missing data about destination operand");
        return ERROR;
    }
    secondOperandType = operandType(word);
    if (secondOperandType == ERROR) {
        printf("Error: missing data");
        return ERROR;
    }
    secondOperand = word; /* save the place of the operand for later */

    word = strtok(NULL, " ");
    if (word) {
        printf("Error: Additional data");
        return ERROR;
    }
    writeBase4(firstOperandType,1,output,FALSE);
    writeBase4(secondOperandType,1,output,FALSE);
    writeBase4(A,1,output,FALSE); /* An instruction is allays absolute*/
    /* Registers are dealt with differently because we might have to put them in the same word*/
    if (operandsCount == 2)
        if (firstOperandType == REGISTER){
            writeReg(firstOperand, secondOperand, output);
        } else {
            if (incodeOperand(firstOperand, output, ext, head) == ERROR)
                return ERROR;
        }

    if (secondOperandType == REGISTER && firstOperandType != REGISTER ){
        writeReg(firstOperand, secondOperand, output);
    } else {
        if (incodeOperand(secondOperand, output, ext, head) == ERROR)
            return ERROR;
    }

}

/* This function receives a operand and returns the memory word required to store it
 * note: this does not deal with registers */
int incodeOperand (char * operand, FILE * output, FILE * ext, sym_ptr head)  {
    int num, type = operandType(operand);
    char firstParam[MAX_LABEL_SIZE], secondParam[MAX_LABEL_SIZE], matrixName[MAX_LABEL_SIZE];
    sym_ptr p;

    switch (type) {
        case IMMEDIATE:
            if (readNum(&num, operand + 1) != ERROR) { /* skip the # and read the number */
                writeAddress(output);
                writeBase4(num,4,output,FALSE);
                writeBase4(A,1,output,FALSE);
            } else {
                printf("Error: after # must follow a number,");
                return ERROR;
            }
            break;
        case VARIABLE:
            if (p = findSym(operand,head)) {
                if (p -> ext == FALSE) {
                    writeAddress(output);
                    writeBase4(p->address,4,output,FALSE);
                    writeBase4(R,1,output,FALSE);
                } else {
                    num = writeAddress(output);
                    writeBase4(0,4,output,FALSE);
                    writeBase4(E,1,output,FALSE);
                    // write to extern
                    fputs(p->label,ext);
                    fputc(' ',ext);
                    writeBase4(num,4,ext, TRUE);
                    fputc('\n',ext);
                }
            } else {
                printf("Error: Use of undefind operand \"%s\"",operand);
                return ERROR;
            }
            break;
        case MATRIX:
            if (readMatrix(firstParam,secondParam,operand) == ERROR)
                return ERROR;
            if (operandType(firstParam) != REGISTER || operandType(secondParam) != REGISTER) {
                printf("Error: Matrix arguments  must be  registers and unlike "
                               "\"%s\"",secondParam);
                return ERROR;
            }
            /* Separate the label of the matrix from the rest */
            for (num = 0; operand[num] != '['; num++)
                matrixName[num] = operand[num];
            matrixName[num] = '\0';
            if (incodeOperand(matrixName, output, ext, head) == ERROR)
                return ERROR;

            writeReg(firstParam,secondParam,output);
            break;
    }
}

/* This command receives the two registers and stores writes them to fp*/
void writeReg (char * source, char * dest, FILE * fp) {
    writeAddress(fp);
    if (operandType(source) == REGISTER) {
        writeBase4(source[1] - '0', 2, fp,FALSE);
    } else {
        writeBase4(0, 2, fp,FALSE);
    }
    if (operandType(dest) == REGISTER) {
        writeBase4(dest[1] - '0', 2, fp,FALSE);
    } else {
        writeBase4(0, 2, fp,FALSE);
    }
    writeBase4(A,1,fp,FALSE);
}

void removeLabel (char * line) {
    char temp [INNER_LINE_SIZE];
    char *p;

    strcpy(temp, line);
    p = strtok(temp, " ");
    if (!p)
        return;
    p = strtok(NULL, " ");
    if (!p)
        return;
    /* Check for a label based on the format from readLine */
    if (strcmp(p, ":") == 0)
        deleteWords(2, line); /* Delete the label form the line */
}