/***************************************************
 * Project: Implementation of the IFJ25 language compiler
 * File   : stack.c
 * Author : Jaroslav Synek (xsynekj00)
 * Date   : 2. 12. 2025
 *
 * Description: Stack implementation
 ***************************************************/



#include "stack.h"
#include "error.h"

Stack* initStack() {
    Stack* stack = malloc(sizeof(Stack));

    if (stack == NULL) {
        fprintf(stderr, "Internal error: malloc failed\n");
        exit(INTERNAL_ERROR);
    }

    stack->count = 0;
    stack->data = NULL;
    return stack;
}

Token* getTop(Stack* stack) {
    for (int i = stack->count - 1; i >= 0; i--) {
        if (stack->data[i]->expr == NULL && stack->data[i]->precedence == '<') {
            return stack->data[i]->token;
        }
    }

    return NULL;
}

void push(Stack* stack, Token* token, char precedence) {
    Node* node = malloc(sizeof(Node));
    if (node == NULL) {
        fprintf(stderr, "Internal error: malloc failed\n");
        exit(INTERNAL_ERROR);
    }

    node->token = token;
    node->expr = NULL;
    node->precedence = precedence;

    stack->count++;
    stack->data = realloc(stack->data, stack->count * sizeof(Node*));
    stack->data[stack->count - 1] = node;
}

void freeStack(Stack* stack) {
    if (stack == NULL) {
        fprintf(stderr, "Internal error: malloc failed\n");
        exit(INTERNAL_ERROR);
    }

    for (int i = 0; i < stack->count; i++) {
        free(stack->data[i]);
    }
    free(stack->data);
    free(stack);
}