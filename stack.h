/***************************************************
 * Project: Implementation of the IFJ25 language compiler
 * File   : stack.h
 * Author : Jaroslav Synek (xsynekj00)
 * Date   : 2. 12. 2025
 *
 * Description: Header file for stack
 ***************************************************/


#ifndef STACK_H
#define STACK_H

#include "lexer.h"
#include "ast.h"

typedef struct {
    Token* token;
    Expr* expr;
    char precedence; 
} Node;

typedef struct {
    int count;
    Node** data;
} Stack;

Stack* initStack();
Token* getTop(Stack* stack);
void push(Stack* stack, Token* token, char precedence);
void pop(Stack* stack);
void freeStack(Stack* stack);

#endif // STACK_H
