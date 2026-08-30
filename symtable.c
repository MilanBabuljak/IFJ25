/***************************************************
 * Project: Implementation of the IFJ25 language compiler
 * File   : symtable.c
 * Author : Milan Babuljak (xbabulm00)
 * Date   : 2. 12. 2025
 *
 * Description: Symbol table implementation - AVL tree
 ***************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "symtable.h"

void initTree(Node **root){
    if (!root) {
        return;
    }
    *root = NULL;
}

int height(Node *N) { 
    if (N == NULL) 
        return 0; 
    return N->height; 
} 

int Max(int a, int b) { 
    return (a > b)? a : b; 
}

Node* search(Node *node, String *symName) {
    const char* str1 = stringGetCStr(symName);
    if (node == NULL) {
        return NULL;
    }

    const char* str2 = stringGetCStr(node->symName);
    int cmp = strcmp(str1, str2);
    if (cmp == 0) {
        return node;
    } else if (cmp < 0) {
        return search(node->LChild, symName);
    } else {
        return search(node->RChild, symName);
    }

    return NULL;
}


// Free the AVL tree
void FreeTree(Node *node) {
    if (!node) {
        return;
    }
    FreeTree(node->LChild);
    FreeTree(node->RChild);
    free(node->label);
    free(node);
}

// Right rotation
void RotateRight(Node **y) {
    Node *x = (*y)->LChild;
    Node *T2 = x->RChild;

    x->RChild = *y;
    (*y)->LChild = T2;

    (*y)->height = Max(height((*y)->LChild), height((*y)->RChild)) + 1;
    x->height = Max(height(x->LChild), height(x->RChild)) + 1;

    *y = x;
}


// Left rotation
void RotateLeft(Node **x) {
    Node *y = (*x)->RChild;
    Node *T2 = y->LChild;

    y->LChild = *x;
    (*x)->RChild = T2;

    (*x)->height = Max(height((*x)->LChild), height((*x)->RChild)) + 1;
    y->height = Max(height(y->LChild), height(y->RChild)) + 1;

    *x = y;
}

// Insert a new symbol into the AVL tree
int Insert(Node **node, String *symName, Type type, Scope scope) {
    if (*node == NULL) {
        *node = malloc(sizeof(Node));
        if (*node == NULL) {
            fprintf(stderr, "Error allocating memory for new node\n");
            exit(EXIT_FAILURE);
        }
        (*node)->symName = symName;
        (*node)->type = type;
        (*node)->scope = scope;
        (*node)->LChild = NULL;
        (*node)->RChild = NULL;
        (*node)->height = 1;
        (*node)->label = NULL;
        return 0;
    }

    const char* str1 = stringGetCStr(symName);
    const char* str2 = stringGetCStr((*node)->symName);
    int cmp = strcmp(str1, str2);

    if (cmp < 0) {
        if (Insert(&(*node)->LChild, symName, type, scope) == -1)
            return -1;
    } else if (cmp > 0) {
        if (Insert(&(*node)->RChild, symName, type, scope) == -1)
            return -1;
    } else {
        return -1; // duplicate
    }

    (*node)->height = 1 + Max(height((*node)->LChild), height((*node)->RChild));
    int balance = height((*node)->LChild) - height((*node)->RChild);

    // Left Left
    if (balance > 1 && strcmp(str1, stringGetCStr((*node)->LChild->symName)) < 0) {
        RotateRight(node);
        return 0;
    }

    // Right Right
    if (balance < -1 && strcmp(str1, stringGetCStr((*node)->RChild->symName)) > 0) {
        RotateLeft(node);
        return 0;
    }

    // Left Right
    if (balance > 1 && strcmp(str1, stringGetCStr((*node)->LChild->symName)) > 0) {
        RotateLeft(&(*node)->LChild);
        RotateRight(node);
        return 0;
    }

    // Right Left
    if (balance < -1 && strcmp(str1, stringGetCStr((*node)->RChild->symName)) < 0) {
        RotateRight(&(*node)->RChild);
        RotateLeft(node);
        return 0;
    }

    return 0;
}


// Debug function: Print the AVL tree
void printTreeHelper(Node *node, int depth, char prefix) {
    if (node == NULL) return;

    // Print right child first (so the tree is rotated)
    printTreeHelper(node->RChild, depth + 1, '/');

    // Print current node with indentation
    for (int i = 0; i < depth; i++) {
        printf("    ");  // 4 spaces per level
    }
    if (depth > 0) {
        printf("%c-- ", prefix);  // show branch
    }

    printf("%s (H=%d)\n", stringGetCStr(node->symName), node->height);

    // Print left child
    printTreeHelper(node->LChild, depth + 1, '\\');
}

// Debug function: Public interface to print the AVL tree
void _printTree_dbg(Node *node) {
    if (node == NULL) {
        printf("<empty tree>\n");
        return;
    }
    printTreeHelper(node, 0, '*');
}
