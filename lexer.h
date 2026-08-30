/***************************************************
 * Project: Implementation of the IFJ25 language compiler
 * File   : lexer.h
 * Author : Milan Babuljak (xbabulm00)
 *        : Jaroslav Synek (xsynekj00)
 * Date   : 2. 12. 2025
 *
 * Description: Header file for lexical analysator
 ***************************************************/

#ifndef LEXER_H
#define LEXER_H

#include <stdio.h>
#include "string.h"

typedef enum {
    TOKEN_IDENTIFIER,
    TOKEN_NUMBER_LITERAL,
    TOKEN_STRING_LITERAL,
    TOKEN_KEYWORD,

    TOKEN_LEFT_PAREN,    // (
    TOKEN_RIGHT_PAREN,   // )
    TOKEN_LEFT_BRACE,    // {
    TOKEN_RIGHT_BRACE,   // }
    TOKEN_DOT,           // .
    TOKEN_MUL,           // *
    TOKEN_DIV,           // /
    TOKEN_ADD,           // +
    TOKEN_SUB,           // -
    TOKEN_LESS,          // <
    TOKEN_LESS_EQUAL,    // <=
    TOKEN_GREATER,       // >
    TOKEN_GREATER_EQUAL, // >=
    TOKEN_EQUAL,         // ==
    TOKEN_NOT_EQUAL,     // !=

    TOKEN_ASSIGN,        // =

    TOKEN_COMMA,         // ,
    TOKEN_NEWLINE,       // \n

    TOKEN_EOF,           // End of file
} token_type;

typedef struct {
    token_type type;
    String *param;
} Token;

// Global: last created/returned token
extern Token previousToken;

Token* getToken(FILE *file);
Token* createToken(token_type type, String *param);
int isKeyword(String *text);
void verifyToken(Token *token);


#endif // LEXER_H