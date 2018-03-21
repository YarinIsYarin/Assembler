#include "mmn14.h"

int readLine (FILE *, char *);

int sizeOf (char * line);
int isLabel (char *, char *, sym_ptr head);
void addToSym (sym_ptr * head, char * label, int address, int ext, int oper);
int storeData (int data[], int dc, char line[]);
int isLlegalLabel (char *label, sym_ptr head);
int isExtern (char line[], sym_ptr * head);
int isDefined (char *line);
int incode (char *line, FILE * output, FILE *ext, sym_ptr head);
void writeBase4 (int num, int digits, FILE * fp, int compact);
int writeAddress (FILE * out);
int isEntry (char line[], sym_ptr head, FILE * ent);
void freeList (sym_ptr p);
void removeLabel (char * line);


int main(int argc, char * argv[]) {

    int ic = 0, dc = 0, errorCount = 0, i, j, lineNumber, readingFile = TRUE, labelFlag;
    char line [INNER_LINE_SIZE], label [MAX_LABEL_SIZE], FileName[MAX_FILE_NAME + 3];
    int data[MAX_DATA_SIZE];
    sym_ptr p, sym_head = NULL;
    FILE *ob, *entry,*as, *ext;
    label[MAX_LABEL_SIZE] = ' ';

    if (argc == 1) {
        printf("Error: missing file name\n");
        return 0;
    }

    /* This allows the program the process more than one file */
    /* Go over each of the parameters of main */
    for (j = 1; j < argc; j++) {
        /* Reset the values between different runs */
        ic = 0, dc = 0, errorCount = 0, readingFile = TRUE,
                sym_head = NULL, label[MAX_LABEL_SIZE] = ' ';
        strcpy(FileName, argv[j]);
        printf("Processing %s...\n",FileName);

        as = fopen(strcat(FileName, ".as"), "r");
        if (as == NULL) {
            printf("Error: can't find \"%s\"\n",FileName);
        } else {
            strcpy(FileName, argv[j]);
            ob = fopen(strcat(FileName, ".ob"), "a");
            strcpy(FileName, argv[j]);
            entry = fopen(strcat(FileName, ".ent"), "a");
            strcpy(FileName, argv[j]);
            ext = fopen(strcat(FileName, ".ext"), "a");

            /* First passage */
            for (lineNumber = 1; readingFile == TRUE; lineNumber++) {
                readingFile = readLine(as, line);
                labelFlag = isLabel(line, label, sym_head); /*  also removes the label */
                /* Make sure the label is valid */
                if (labelFlag == ERROR) {
                    errorCount++;
                    printf(" at line %d\n", lineNumber);
                    /* The rest of the error message is printed by isLabel */
                }

                if (sizeOf(line) && sizeOf(line) != ERROR) { /* Deal with instructions */
                    if (labelFlag)
                        addToSym(&sym_head, label, ic + STARTING_ADDRESS, FALSE, TRUE);
                    ic += sizeOf(line);
                } else { /* Deal with memory allocation */
                    if (storeData(data, dc, line) == ERROR) {
                        printf(" at line %d\n", lineNumber);
                        /* The rest of the error message is printed by storeData */
                        errorCount++;
                    } else if (storeData(data, dc, line)) {
                        if (labelFlag)
                            addToSym(&sym_head, label, dc, FALSE, FALSE);
                        dc += storeData(data, dc, line);
                    } else {
                        /* The there is a label to the likes of an empty line */
                        if (labelFlag) {
                            printf("Warning: label to a line without an instruction \\ "
                                           "use of the label at line %d\n",lineNumber);
                        addToSym(&sym_head, label, ic + STARTING_ADDRESS, FALSE, TRUE);}
                    }
                    if (isExtern(line, &sym_head) == ERROR) { /* Deal with .extern*/
                        printf(" at line %d\n", lineNumber);
                        errorCount++;
                    }
                }
            }
            /* Write ic and dc*/
            writeBase4(ic, ic, ob, TRUE);
            fputc('\t', ob);
            writeBase4(dc, dc, ob, TRUE);
            /* Update the symbles in data by the offset */
            p = sym_head;
            while (p) {
                if (p->operation == FALSE && p->ext == FALSE) {
                    p->address += ic + STARTING_ADDRESS;
                }
                p = p->next;
            }

            /* Second passage */
            readingFile = TRUE;
            rewind(as);
            for (lineNumber = 1; readingFile == TRUE; lineNumber++) {
                readingFile = readLine(as, line);
                removeLabel(line);
                /* if the command has an opcode */
                if (isDefined(line)) {
                    /* Only process if the line is enmty*/
                    if (sizeOf(line)) {
                        if (incode(line, ob, ext, sym_head) == ERROR) {
                            /* The rest of the error message is printed by incode */
                            printf("%d\n", lineNumber);
                            errorCount++;
                        }
                    } else {
                        if (isEntry(line, sym_head, entry) == ERROR) {
                            errorCount++;
                            /* The rest of the error message is printed by isEntry */
                            printf("%d\n", lineNumber);
                        }
                    }
                } else { /* error message for unknown commands */
                    printf("Error: Undefined instruction at line %d\n", lineNumber);
                    errorCount++;
                }
            }
            /* Add the values in data to the end of instructions */
            for (i = 0; i < dc; i++) {
                writeAddress(ob);
                /*printf("the nume is %d\n",data[i]);*/
                /* We save the data in this form of based 4 as instructed */
                writeBase4(data[i], 5, ob, FALSE);
            }

            /* Delete unnecessary files */
            if (ftell(entry) == 0) {
                fclose(entry);
                strcpy(FileName, argv[j]);
                remove(strcat(FileName, ".ent"));
                remove(FileName);
            } else {
                fclose(entry);
            }
            if (ftell(ext) == 0) {
                fclose(ext);
                strcpy(FileName, argv[j]);
                remove(strcat(FileName, ".ext"));
                remove(FileName);
            } else {
                fclose(ext);
            }

            /* Finish the program */
            freeList(sym_head);
            fclose(as);
            fclose(ob);
        }

        /* If the input file (.as) had errors, the output files are useless,
         and will be deleted */
        if (errorCount) {
            strcpy(FileName, argv[j]);
            remove(strcat(FileName, ".ob"));
            strcpy(FileName, argv[j]);
            remove(strcat(FileName, ".ent"));
            strcpy(FileName, argv[j]);
            remove(strcat(FileName, ".ext"));
        }
    }
    return 1;
}

/* Reads a line from fp, and stored it in out in a format that is easy to work with (all
   of the different elements of the line are separated by ONE ' '), handles comments,
   Replaces all the spaces in strings with " so we can easly
   and returns FALSE if reached EOF, otherwise returns TRUE */
int readLine (FILE * fp, char * out) {
    int i =0, stringFlag = FALSE, prev = '\0', in = fgetc(fp);

    while (in != '\n'  && in != EOF) {
        if (!stringFlag) {
            if (in == '\t') {
                in = ' ';
            }
            if (in == ' ') {
                if (prev != ' ' && prev != '\0' && prev != '[') {
                    out[i++] = ' ';
                    prev = in;
                }
            } else if (in == ':' || in == ',' || in == '\"') {
                if (prev != ' ' && prev != '\0')
                    out[i++] = ' ';
                out[i++] = in;
                out[i++] = ' ';
                prev = ' ';
                if (in == '\"')
                    stringFlag = TRUE;

            } else if (in == ']') {
                if (prev == ' ')
                    i--;
                out[i++] = in;
                out[i++] = ' ';
                prev = ' ';
            } else if (in == '[') {
                if (prev == ' ')
                    i--;
                out[i++] = in;
                prev = ' ';
            } else {
                out[i++] = in;
                prev = in;
            }
            in = fgetc(fp);
        } else {
            out[i] = in;
            /* we replace ' ' with " inside of strings so that we can still use strtok */
            if (in == ' ')
                out[i] = '\"';
            i++;
            in = fgetc(fp);
            if (in == '\"') {
                stringFlag = FALSE;
                out[i++] = ' ';
                prev = ' ';
            }
        }

    }
    /* Check for comment */
    if (out[0] == ';')
        strcpy(out,"\0");
    out [i] = '\0';
    if (in == EOF)
        return FALSE;
    return TRUE;
}   

/* Free the given list using recursion */
void freeList (sym_ptr p) {
    if (!p) /* Base case */
        return;
    freeList(p->next); /* Step */
    free(p->label);
    free(p);
}
