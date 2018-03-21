#include "mmn14.h"
#include <ctype.h>

int isDefined (char *line);
int isLlegalLabel (char * label, sym_ptr head);
int isLabel (char * line,char * label, sym_ptr head);
int sizeOf (char * line);
int storeData (int data[], int dc, char line[]);
int isExtern (char line[], sym_ptr * head);

int readMatrix (char *firstParam, char *secondParam, char * matrix);
void deleteWords (int i, char *line);
sym_ptr findSym (char * name, sym_ptr head);
void writeBase4 (int num, int digits, FILE * fp, int);
int isMat (char * word);
int howManyOperands (char *instruction);
int operandType (char * operands);
int readNum (int *out, char * read);
void addToSym (sym_ptr * head, char * label, int address, int ext, int operation);


/* Receives a line without a label (in the format of the output readLine) and returns TRUE
 * if the first word is defined in asm, returns false otherwise */
int isDefined (char *line) {
    char * defined[] = { "mov", "cmp", "add", "sub", "not", "clr", "lea", "inc", "dec",
                         "jmp", "bne", "red", "prn", "sjr", "rts", "stop", ".string",
                         ".data", ".entry", ".extern", ".mat", "$"};
    int i;
    char * firstWord, temp[INNER_LINE_SIZE];
    strcpy(temp,line);
    firstWord = strtok(temp, " ");
    if (!firstWord)
        return TRUE;
    if (isMat(firstWord))
        return TRUE;

    /* Compare the first word in the line to all the defined key-words */
    for (i = 0; strcmp(defined[i],"$"); i++)
        if (strcmp(defined[i],firstWord) == 0) {
            return TRUE;
        }
    return FALSE;
}

/* Makes sure a given label follows the syntax, returns ERROR if not */
int isLlegalLabel (char * label, sym_ptr head) {
    int i;

    if (!isalpha(label[0])) {
        printf("Error: label doesn't start with a letter");
        return ERROR;
    }
    /* Make sure the label is not a reserved word*/
    if (isDefined(label)){
        printf("Error: Illegal label");
        return ERROR;
    }
    /* Make sure it's not a register */
    if (label[0] == 'r')
        if(label[1] >= '0' && label[1] <= MAX_REGISTER+'0')
            if (label[2] == '\0') {
                printf("Error: label have the same name as a register,");
                return ERROR;
            }
    /* Make sure it only contains alphanumeric chars */
    for (i = 0; label[i] != '\0'; i++)
        if (!isalnum(label[i])) {
            printf("Error: Label may only contain alphanumeric chars");
            return ERROR;
        }
    /* Make sure the label is new */
    if (findSym(label, head) != NULL) {
        printf("Error: every label must be unique, error occurred");
        return ERROR;
    }
    return TRUE;
}

/* Receives a line (read by readLine) and saves it into label, removes the label from the line */
int isLabel (char * line,char * label, sym_ptr head) {
    char temp [INNER_LINE_SIZE];
    char *p;

    strcpy(temp, line);
    p = strtok(temp, " ");
    if (!p)
        return FALSE;
    p = strtok(NULL, " ");
    if (!p)
        return FALSE;
    /* Check for a label based on the format from readLine */
    if (strcmp(p, ":") == 0) {
        strcpy(label, temp);
        /* Delete the label form the line */
        deleteWords(2, line);
        return isLlegalLabel(label, head);
    }
    return FALSE;
}

/* Gets a line and returns how many words (in memory) the instruction in the line takes*/
int sizeOf (char * line) {
    char temp[INNER_LINE_SIZE];
    char *word; /*points to the start of the word we are currently processing */
    int operandsCount, firstParam, secondParam, returnVal = 0;

    strcpy(temp, line);
    word = strtok(temp, " ");
    if (word == NULL)
        return 0;
    operandsCount = howManyOperands(word);
    if (operandsCount == ERROR)
        return 0;
    if (operandsCount == 0)
        return 1;

    /* Read first param */
    word = strtok(NULL, " ");
    /* if there is no first param then there is an error and there is no need to calc
    the size of the instruction */
    if (word == NULL || strcmp(word, ",") == 0)
        return ERROR;
    firstParam = operandType(word);
    if (firstParam == ERROR)
        return ERROR;

    /* They all take one word */
    if (firstParam == IMMEDIATE || firstParam == REGISTER || firstParam == VARIABLE) {
        returnVal += 1;
    } else {
    returnVal += 2; /* only other option is a matrix*/
    }
    if (operandsCount == 1) {

        return 1 + returnVal;
    }

    /* operandCount must be 2 */
    /* check fof comma */
    word = strtok(NULL, " ");
    if (!word)
        return ERROR;
    if (strcmp(word, ",") != 0)
        return ERROR;

    word = strtok(NULL, " ");
    secondParam = operandType(word);
    if (secondParam == ERROR)
        return ERROR;

    if ( firstParam == REGISTER && secondParam == REGISTER)
        return 2;

    if (secondParam == IMMEDIATE || secondParam == REGISTER || secondParam == VARIABLE) {
        returnVal += 1;
    } else {
        returnVal += 2; /* only other option is a matrix*/
    }
    return returnVal + 1;
}

/* Deals with .data, .string and .mat, returns the change to dc (if it's not one of those
 * commands it returns the 0) */
int storeData (int data[], int dc, char line[]) {
    char temp [INNER_LINE_SIZE];
    char *word; /*points at the current word in the line we are processing */
    int toAdd ,newDc = dc;

    strcpy(temp,line);
    word = strtok(temp, " ");
    if (!word)
        return 0;

    if (strcmp(word,".data") == 0) {
        int errorFlag = FALSE;
        /* store all of the numbers in the line */
        while (word) {
            if (errorFlag) {
                printf("Error: Missing comma");
                return ERROR;
            }
            word = strtok(NULL, " ");
            if (readNum(&toAdd, word) == ERROR) {
                printf("Error: Illegal number");
                return ERROR;
            }
            data[newDc++] = toAdd;
            word = strtok(NULL, " ");
            if (word)
                if (strcmp(word, ",") != 0)
                    errorFlag = TRUE;
        }
        return newDc - dc;
    }

    if (strcmp(word,".string") == 0) {
        int i;
        word = strtok(NULL, " ");
        if (strcmp(word,"\"") != 0) {
            printf("Error: Missing quotations marks");
            return ERROR;
        }
        /* store all of the chars in the string */
        word = strtok(NULL, " ");
        if (!word) {
            printf("Error: Missing quotations marks");
            return ERROR;
        }
        if (strcmp(word,"\"") != 0) {
            for (i = 0; word[i] != '\0'; i++) {
                data[newDc] = word[i];
                if (data[newDc] == '\"') /* In readLine we replaced all spaces inside strings with " */
                    data[newDc] = ' ';
                newDc++;
            }
            word = strtok(NULL, " ");
            if (strcmp(word,"\"") != 0) {
                printf("Error: Missing quotations marks");
                return ERROR;
            }
        }
        data[newDc++] = '\0';
        return newDc - dc;
    }

    if (isMat(word)) {
        char colSizeString[MAX_LABEL_SIZE],  rowSizeString[MAX_LABEL_SIZE];
        int colSize, rowSize;

        if (readMatrix(rowSizeString,colSizeString,word) == ERROR)
            return ERROR;
        if (readNum(&rowSize,rowSizeString) == ERROR) {
            printf("Error: Ileagal row size");
            return ERROR;
        }
        if (readNum(&colSize,colSizeString) == ERROR) {
            printf("Error: Ileagal row size");
            return ERROR;
        }

        /* store the values in the matrix */
        word = strtok(NULL, " ");
        while (word) {
            if (readNum(&toAdd, word) == ERROR) {
                printf("Error: Illegal number");
                return ERROR;
            }
            data[newDc++] = toAdd;
            word = strtok(NULL, " ");
            if (word) {
                if (strcmp(word, ",") != 0) {
                    printf("Error: Missing comma");
                    return ERROR;
                }
                word = strtok(NULL, " ");
                if (!word) {
                    printf("Error excessive commas");
                    return ERROR;
                }
            }
        }
        /* check if the user entered more values then the matrix size */
        if (newDc - dc > colSize*rowSize) {
            printf("Error: Too many values");
            return ERROR;
        }
        /* Fill the rest of the allocated space*/
        while (newDc - dc < colSize * rowSize)
            data[newDc++] = 0;

        return colSize * rowSize;
    }
    return 0;
}

/* Deals with .extern, returns TRUE if it did, FALSE if there wasn't
   an extern in that line and ERROR if the line is an extern but invalid */
int isExtern (char line[], sym_ptr * head) {
    char temp[INNER_LINE_SIZE];
    char *word; /*points at the current word in the line we are processing */

    strcpy(temp,line);
    word = strtok(temp, " ");
    if (!word)
        return FALSE;
    if (strcmp(word,".extern") == 0) {
        word = strtok(NULL, " ");
        if (!word) {
            printf("Error: missing name");
            return ERROR;
        }
        if (!isLlegalLabel(word, *head)) {
            printf("Error: Illegal label name");
            return ERROR;
        }
        addToSym(head, word, FALSE, TRUE, FALSE);
        return TRUE;
    }
    return FALSE;
}

/* Checks if the current command is .mat*/
int isMat (char * word) {
    char temp[strlen(word)+1];

    if (strlen(word) < 4)
        return FALSE;
    strcpy(temp,word);
    temp[4] = '\0';

    return strcmp(temp,".mat") == 0;
}