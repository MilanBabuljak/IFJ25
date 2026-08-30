/***************************************************
 * Project: Implementation of the IFJ25 language compiler
 * File   : debug_utils.c
 * Author : Milan Babuljak (xbabulm00)
 * Date   : 2. 12. 2025
 *
 * Description: Implementation of debug utilities for testing
 ***************************************************/

// zajebem sa

#include "lexer.h"
#include "string.h"
#include <stdio.h>

const char* token2String(token_type type) {
    switch (type) {
        case TOKEN_IDENTIFIER: return "TOKEN_IDENTIFIER";
        case TOKEN_NUMBER_LITERAL: return "TOKEN_NUMBER_LITERAL";
        case TOKEN_STRING_LITERAL: return "TOKEN_STRING_LITERAL";
        case TOKEN_KEYWORD: return "TOKEN_KEYWORD";
        case TOKEN_LEFT_PAREN: return "TOKEN_LEFT_PAREN";
        case TOKEN_RIGHT_PAREN: return "TOKEN_RIGHT_PAREN";
        case TOKEN_LEFT_BRACE: return "TOKEN_LEFT_BRACE";
        case TOKEN_RIGHT_BRACE: return "TOKEN_RIGHT_BRACE";
        case TOKEN_DOT: return "TOKEN_DOT";
        case TOKEN_MUL: return "TOKEN_MUL";
        case TOKEN_DIV: return "TOKEN_DIV";
        case TOKEN_ADD: return "TOKEN_ADD";
        case TOKEN_SUB: return "TOKEN_SUB";
        case TOKEN_LESS: return "TOKEN_LESS";
        case TOKEN_LESS_EQUAL: return "TOKEN_LESS_EQUAL";
        case TOKEN_GREATER: return "TOKEN_GREATER";
        case TOKEN_GREATER_EQUAL: return "TOKEN_GREATER_EQUAL";
        case TOKEN_EQUAL: return "TOKEN_EQUAL";
        case TOKEN_NOT_EQUAL: return "TOKEN_NOT_EQUAL";
        case TOKEN_ASSIGN: return "TOKEN_ASSIGN";
        case TOKEN_COMMA: return "TOKEN_COMMA";
        case TOKEN_NEWLINE: return "TOKEN_NEWLINE";
        case TOKEN_EOF: return "TOKEN_EOF";
        default: return "UNKNOWN_TOKEN";
    }
}

void printToken(Token *token) {
    if (!token) return;
    printf("Debug: ");
    const char *s = stringGetCStr(token->param);
    printf("Token Type: %s, Data: ", token2String(token->type));
    if (s) {
        for (const unsigned char *p = (const unsigned char*)s; *p; ++p) {
            unsigned char c = *p;
            if (c == ' ' || c == '\n' || c == '\t' || c == '\r' || c == '\v' || c == '\f') {
                printf("(%d)", c);
            } else {
                putchar(c);
            }
        }
    } else {
        printf("(null)");
    }
    printf("\n");
    return;
}