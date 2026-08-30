/***************************************************
 * Project: Implementation of the IFJ25 language compiler
 * File   : lexer.c
 * Author : Milan Babuljak (xbabulm00)
 *        : Jaroslav Synek (xsynekj00)
 * Date   : 2. 12. 2025
 *
 * Description: Lexical analyzer implementation
 ***************************************************/


#include "lexer.h"
#include "string.h"
#include "error.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

Token previousToken; 
Token stack;
int needToSendStack = 0;

int oldC;
int needToReadOldC = 0;


Token* getToken(FILE *file) {


    if (needToSendStack) {
        needToSendStack = 0;
        return &stack;
    }

    int c;
    if (needToReadOldC) {
        needToReadOldC = 0;
        c = oldC;
    } else c = fgetc(file);
    
    if (c == '\n') return createToken(TOKEN_NEWLINE, NULL);

    while (isspace(c) ) { 
        c = fgetc(file);
        if (c == '\n') return createToken(TOKEN_NEWLINE, NULL);
    }

    if (c == EOF) return createToken(TOKEN_EOF, NULL);
    if (c == ')') return createToken(TOKEN_RIGHT_PAREN, NULL);
    if (c == '(') return createToken(TOKEN_LEFT_PAREN, NULL);
    if (c == '{') return createToken(TOKEN_LEFT_BRACE, NULL);
    if (c == '}') return createToken(TOKEN_RIGHT_BRACE, NULL);
    if (c == ',') return createToken(TOKEN_COMMA, NULL);
    if (c == '.') return createToken(TOKEN_DOT, NULL);
    if (c == '+') return createToken(TOKEN_ADD, NULL);
    if (c == '*') return createToken(TOKEN_MUL, NULL);
        
    if (c == '-') {
        int c2 = fgetc(file);
        if (c2 == EOF) {
        return createToken(TOKEN_SUB, NULL);
        }

        if (isdigit(c2)) {
        if (!(previousToken.type == TOKEN_NUMBER_LITERAL ||
                previousToken.type == TOKEN_IDENTIFIER ||
                previousToken.type == TOKEN_RIGHT_PAREN ||
                previousToken.type == TOKEN_RIGHT_BRACE)) {

            ungetc(c2, file);

            String *zero_str = stringInit();
            stringAppendChar(zero_str, '0');

            stack.type = TOKEN_SUB;
            stack.param = NULL;
            needToSendStack = 1;

            return createToken(TOKEN_NUMBER_LITERAL, zero_str);
        }
        }

        ungetc(c2, file);
        return createToken(TOKEN_SUB, NULL);
    }

    // -- < and <=
    if (c == '<') {
        c = fgetc(file);
        if (c == '=') {
            return createToken(TOKEN_LESS_EQUAL, NULL);
        } else {
            ungetc(c, file);
            return createToken(TOKEN_LESS, NULL);
        }
    }

    // -- > and >=
    if (c == '>') {
        c = fgetc(file);
        if (c == '=') {
            return createToken(TOKEN_GREATER_EQUAL, NULL);
        } else {
            ungetc(c, file);
            return createToken(TOKEN_GREATER, NULL);
        }
    }

    // -- !=
    if (c == '!') {
        c = fgetc(file);
        if (c == '=') {
            return createToken(TOKEN_NOT_EQUAL, NULL);
        } else {
            perror("Lexical Error: Unexpected character '!'");
            exit(LEXICAL_ERROR);
        }
    }

    // -- == and =
    if (c == '=') {
        c = fgetc(file);
        if (c == '=') {
            return createToken(TOKEN_EQUAL, NULL);
        } else {
            ungetc(c, file);
            return createToken(TOKEN_ASSIGN, NULL);
        }
    }

    // --- NUMBER (NORMAL, HEX, DECIMAL, EXPONENTIAL) ---
    if (isdigit(c)) {
        String *buffer = stringInit();   
        do {
            stringAppendChar(buffer, c);
            c = fgetc(file);
        } while (isdigit(c) || c == 'x' || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F') || c == '.' || c == '+' || c == '-');
        ungetc(c, file);
        return createToken(TOKEN_NUMBER_LITERAL, buffer);
    }

    if (c == '/') {
        c = fgetc(file);

        // --- SINGLE LINE COMMENT
        if (c == '/') {
            while (c != '\n' && c != EOF) {
                c = fgetc(file);
            }

            if (c == EOF) {
                return createToken(TOKEN_EOF, NULL);
            } else {
                if (c == '\n') {
                    return createToken(TOKEN_NEWLINE, NULL);
                }
                ungetc(c, file);
                return getToken(file);
            }

            // prev = previous char
            // c is lookahead fun
        } else if (c == '*') {
            int prev = 0;
            int commentCounter = 1;
            while (1) {
                c = fgetc(file);
                if (c == EOF) {
                    printf("Lexical Error: Unterminated multi-line comment at end of file");
                    exit(LEXICAL_ERROR);
                }
                if (prev == '/' && c == '*') {
                    commentCounter++;
                }
                if (prev == '*' && c == '/') {
                    commentCounter--;
                    if (commentCounter == 0) break;
                }
                prev = c;
            }
            return getToken(file);
        } else {
            ungetc(c, file);
            return createToken(TOKEN_DIV, NULL);
        }

    }


        // --- STRING LITERAL ---
        if (c == '"') {
            String *buffer = stringInit();
            if (!buffer) return NULL; 
            c = fgetc(file);

            if (c == '"') {
                c = fgetc(file);
                if (c == '"') {
                    // MULTILINE STRING LITERAL
                    while (1) {
                        c = fgetc(file);
                        if (c == EOF) {
                            perror("Lexical Error: Unterminated string literal at the end of file");
                            exit(LEXICAL_ERROR);
                        }

                        if (c == '"') {
                            c = fgetc(file);
                            if (c == '"') {
                                c = fgetc(file);
                                if (c == '"') {

                                    // --- Multiline String Cleanup ---
                                    
                                    // 1. Remove initial whitespace + newline
                                    int startIdx = 0;
                                    while (startIdx < buffer->length && isblank(buffer->str[startIdx])) {
                                        startIdx++;
                                    }
                                    if (startIdx < buffer->length && buffer->str[startIdx] == '\n') {
                                        int removeLen = startIdx + 1;
                                        int newLen = buffer->length - removeLen;
                                        memmove(buffer->str, buffer->str + removeLen, newLen);
                                        buffer->str[newLen] = '\0';
                                        buffer->length = newLen;
                                    }

                                    // 2. Remove trailing newline + whitespace
                                    if (buffer->length > 0) {
                                        int endIdx = buffer->length - 1;
                                        while (endIdx >= 0 && isblank(buffer->str[endIdx])) {
                                            endIdx--;
                                        }
                                        if (endIdx >= 0 && buffer->str[endIdx] == '\n') {
                                            buffer->str[endIdx] = '\0';
                                            buffer->length = endIdx;
                                        }
                                    }

                                    break; // End of multiline string
                                } else {
                                    stringAppendChar(buffer, '"');
                                    stringAppendChar(buffer, '"');
                                    ungetc(c, file);
                                }
                            } else {
                                stringAppendChar(buffer, '"');
                            }
                        } 

                        stringAppendChar(buffer, c);

                    }

                    return createToken(TOKEN_STRING_LITERAL, buffer);
                     

                } else {
                    oldC = c;
                    needToReadOldC = 1;
                    return createToken(TOKEN_STRING_LITERAL, buffer);
                }
            }

         
            // --- SINGLELINE STRING LITERAL
            while (c != '"') {
                if (c == EOF) {
                    perror("Lexical Error: Unterminated string literal at end of file");
                    exit(LEXICAL_ERROR);
                }
                if (c == '\n') {
                    perror("Lexical Error: Unterminated string literal at end of line");
                    exit(LEXICAL_ERROR);
                }

                if (c == '\\') {
                    c = fgetc(file);
                    if (c == EOF) {
                        perror("Lexical Error: Unterminated string literal at end of file");
                        exit(LEXICAL_ERROR);
                    }
                    if (c == '\n') {
                        perror("Lexical Error: Unterminated string literal at end of line");
                        exit(LEXICAL_ERROR);
                    }

                    if (c == 'n') {
                        stringAppendChar(buffer, '\n');
                    } else if (c == 't') {
                        stringAppendChar(buffer, '\t');
                    } else if (c == '"') {
                        stringAppendChar(buffer, '"');
                    } else if (c == '\\') {
                        stringAppendChar(buffer, '\\');
                    } else {
                        printf("Lexical Error: Unknown escape sequence \\%c\n", c);
                        exit(LEXICAL_ERROR);
                    }
                } else {
                    stringAppendChar(buffer, c);
                }
                c = fgetc(file);
            }
    
            if (c != '"') {
                free(buffer);
                return NULL;
            }

            return createToken(TOKEN_STRING_LITERAL, buffer);
        }


    // --- IDENTIFIER or KEYWORD ---
    if (isalpha(c) || c == '_') {
        String *buffer = stringInit();
        if (!buffer) return NULL;

        do {
            stringAppendChar(buffer, c);
            c = fgetc(file);
        } while (isalnum(c) || c == '_');
        ungetc(c, file);

        Token *token;
        if (isKeyword(buffer)) {
            token = createToken(TOKEN_KEYWORD, buffer);
        } else {
            token = createToken(TOKEN_IDENTIFIER, buffer);
        }
        return token;
    }

    fprintf(stderr, "Lexer error: Unknown char - %c ascii id %d\n", c, c);
    exit(LEXICAL_ERROR);

}


Token* createToken(token_type type, String *text) {

    Token *token = malloc(sizeof(Token));
    if (token == NULL){
        fprintf(stderr, "Internal error: Memory allocation failed.\n");
        exit(INTERNAL_ERROR);
    }

    token->type = type;
    token->param = text;
    previousToken = *token;

    verifyToken(token);
    return token;
}


int isKeyword(String *text) {
    const char *keywords[] = {
        "class", "if", "else", "is", "null", "return", "var", "while", "Ifj",
        "static", "true", "false", "Num", "String", "Null"
    };

    char *textCStr = stringGetCStr(text);

    for (int i = 0; i < 15; i++) {  // wasted hours looking for this misstake: 5
        if (strcmp(textCStr, keywords[i]) == 0) {
            return 1;
        }
    }

    return 0;
}

void verifyToken(Token *token) {

    if (!token) {
        fprintf(stderr, "Lexer error: getToken returned NULL\n");
        exit(LEXICAL_ERROR);
    }

    // Number checks
    if (token->type == TOKEN_NUMBER_LITERAL) {
        char *numStr = stringGetCStr(token->param);

        // --- RULE: Hexadecimal numbers must start with 0x ---

        char hexPrefix[3] = {numStr[0], numStr[1], '\0'};
        if (strcmp(hexPrefix, "0x") == 0) {

            // --- RULE: Hexadecimal numbers cannot have decimal points ---

            if (strchr(numStr, '.') != NULL) {
                fprintf(stderr, "Lexical Error: Invalid hexadecimal number format (decimal point found)");
                exit(LEXICAL_ERROR);
            }

            // --- RULE: Hexadecimal numbers must have at least one number after 0x
            char c = numStr[2];
            if (!(isdigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))) {
                fprintf(stderr, "Lexical Error: Invalid hexadecimal number format (empty after prefix)");
                exit(LEXICAL_ERROR);
            }
            
            // Conversion hex to decimal, since apparently int@0x00FF does not work in IFJ25
            int decimalValue = 0;
            for (int i = 2; numStr[i] != '\0'; i++)
            {
                char c = numStr[i];
                int digitValue;

                if (isdigit(c)) {
                    digitValue = c - '0';
                } else if (c >= 'a' && c <= 'f') {
                    digitValue = c - 'a' + 10;
                } else if (c >= 'A' && c <= 'F') {
                    digitValue = c - 'A' + 10;
                } else {
                    // Byproduct of earlier checks, why not let it run
                    fprintf(stderr, "Lexical Error: Invalid character in hexadecimal number\n");
                    exit(LEXICAL_ERROR);
                }

                decimalValue = decimalValue * 16 + digitValue;
            }

            // Replace token string with decimal representation
            // Too late to re-engineer tokens, so were just overwriteing the string
            stringClear(token->param);
            char decimalStr[20];
            sprintf(decimalStr, "%d", decimalValue);
            for (char *p = decimalStr; *p != '\0'; p++) {
                stringAppendChar(token->param, *p);
            }

            return;
        }

        // --- RULE: Multiple decimal points are not allowed ---
        int dotCount = 0;
        for (char *p = numStr; *p != '\0'; p++) {
            if (*p == '.') {
                dotCount++;
                if (dotCount > 1) {
                    fprintf(stderr, "Lexical Error: Invalid number format (multiple decimal points)");
                    exit(LEXICAL_ERROR);
                }
            }
        }

        // --- RULE: Decimal numbers cannot have hexadecimal characters, only E and + and -, at that point they become exponential ---
        for (char *p = numStr; *p != '\0'; p++) {
            if (isalpha(*p) && *p != 'e' && *p != 'E') {
                fprintf(stderr, "Lexical Error: Invalid character in decimal number");
                exit(LEXICAL_ERROR);
            }
        }

        // -- RULE: Normal and decimal numbers cannot have + or - anywhere ---
        if (strchr(numStr, '+') != NULL || strchr(numStr, '-') != NULL) {
            int e_found = 0;
            for (char *p = numStr; *p != '\0'; p++) {
                if (*p == 'e' || *p == 'E') {
                    e_found = 1;
                    break;
                }
            }
            if (!e_found) {
                fprintf(stderr, "Lexical Error: Invalid character '+' or '-' in number");
                exit(LEXICAL_ERROR);
            }
        }

        
        // --- RULE: Exponential notation must have at least one E or e, one . and one digit before E ---
        int eCount = 0;
        int countSign = 0;
        int countDots = 0;
        for (char *p = numStr; *p != '\0'; p++) {
            if (*p == 'e' || *p == 'E') {
                eCount++;
                if (eCount > 1) {
                    fprintf(stderr, "Lexical Error: Invalid number format (multiple exponents)");
                    exit(LEXICAL_ERROR);
                }
                if (*(p + 1) == '+' || *(p + 1) == '-') {
                    countSign++;
                    p++;
                }
                if (!isdigit(*(p + 1))) {
                    fprintf(stderr, "Lexical Error: Invalid exponent format");
                    exit(LEXICAL_ERROR);
                }
            } else if (*p == '.') {
                countDots++;
                if (countDots > 1) {
                    fprintf(stderr, "Lexical Error: Invalid number format (multiple decimal points)");
                    exit(LEXICAL_ERROR);
                }
            } else if (!isdigit(*p) && *p != '+' && *p != '-') {
                fprintf(stderr, "Lexical Error: Invalid character in number");
                exit(LEXICAL_ERROR);
            }
        }

        // Convertion from exponential to decimal
        if (eCount == 1) {
            char *endPtr;
            double value = strtod(numStr, &endPtr);
            if (endPtr == numStr) {
                fprintf(stderr, "Lexical Error: Invalid exponential number format");
                exit(LEXICAL_ERROR);
            }

            // Replace token string with decimal representation
            stringClear(token->param);
            char decimalStr[400];
            sprintf(decimalStr, "%.20f", value); 
            
            char *p = decimalStr + strlen(decimalStr) - 1;
            while (p > decimalStr && *p == '0') {
                *p = '\0';
                p--;
            }
            if (*p == '.') {
                strcat(decimalStr, "0");
            }

            for (char *p = decimalStr; *p != '\0'; p++) {
                stringAppendChar(token->param, *p);
            }

            return;
        }

    }

}