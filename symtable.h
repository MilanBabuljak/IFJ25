/***************************************************
 * Project: Implementation of the IFJ25 language compiler
 * File   : symtable.h
 * Author : Milan Babuljak (xbabulm00)
 * Date   : 2. 12. 2025
 *
 * Description: Header file for symbol table
 ***************************************************/


#ifndef SYMTABLE_H
#define SYMTABLE_H

#include <stdlib.h>
#include "string.h"

typedef struct Node Node;

typedef enum {
    DTYPE_NUM,
    DTYPE_STRING,
    DTYPE_NULL,
    TYPE_FUNC
} Type;

typedef enum {
    GLOBAL,
    LOCAL
} Scope;

struct Node {
    String *symName;
    Type type;
    Scope scope;
    struct Node *LChild;
    struct Node *RChild;
    int height;
    char *label;
};

void initTree(Node **root);
int height(Node *N);
int Max(int a, int b);
int Insert(Node **node, String *symName, Type type, Scope scope);
Node* search(Node *node, String *symName);
void FreeTree(Node *node);
void RotateRight(Node **y);
void RotateLeft(Node **x);
void printTreeHelper(Node *node, int depth, char prefix);
void _printTree_dbg(Node *node);




#endif // SYMTABLE_H