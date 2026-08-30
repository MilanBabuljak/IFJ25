/***************************************************
 * Project: Implementation of the IFJ25 language compiler
 * File   : debug_utils.c
 * Author : Milan Babuljak (xbabulm00)
 * Date   : 2. 12. 2025
 *
 * Description: Header file for implementation of debug utilities for testing
 ***************************************************/


#ifndef DEBUG_UTILS_H
#define DEBUG_UTILS_H

#include "lexer.h"

const char* token2String(token_type type);
void printToken(Token *token);

#endif // DEBUG_UTILS_H