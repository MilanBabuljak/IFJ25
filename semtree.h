/***************************************************
 * Project: Implementation of the IFJ25 language compiler
 * File   : semtree.c
 * Author : Milan Babuljak (xbabulm00)
 *        : Jaroslav Synek (xsynekj00)   
 * Date   : 2. 12. 2025
 *
 * Description: Header file for implementation of semantic tree (scope tree) 
 ***************************************************/


#ifndef SEMTREE_H
#define SEMTREE_H

#include <stddef.h>
#include <stdbool.h>
#include "string.h"

struct Node;

typedef struct getterListItem_t {
    String* id;
    struct getterListItem_t* nextGetter;
} getterListItem;
typedef struct setterListItem_t {
    String* id;
    struct setterListItem_t* nextSetter;
} setterListItem;
typedef struct {
    getterListItem* getters;
    setterListItem* setters;
} gettersAndSetters;

typedef struct semtree_node {
    String *name;
	struct Node *symTab;
	struct semtree_node *parent;
	struct semtree_node **children;
	size_t childCount;
	size_t childIter;
	size_t scopeId;
	size_t indexInParent;
	char *prefix;
    getterListItem* getters;
    setterListItem* setters;
} semtree_node_t;

typedef void (*semtreeSymbolIterCB)(String *name, const char *label, void *userData);
semtree_node_t *semtreeCreateNode(String *name);
semtree_node_t *semtreeCreateChild(semtree_node_t *parent, String *name);
int semtreeAddChild(semtree_node_t *parent, semtree_node_t *child);
int semtreeAddsymName(semtree_node_t *node, String *symName);
bool semTreeSymbolDefinedHere(const semtree_node_t *node, String *symName);
bool semtreeResolveSymbol(const semtree_node_t *node, String *symName);
void semtreeFree(semtree_node_t *node);
void bottomTopPrint(semtree_node_t *node);

void semtreeResetCodegenCursors(semtree_node_t *node);
int semtreePrepareCodegenLabels(semtree_node_t *node);
const char *semtreeLookupLabel(const semtree_node_t *node, String *symName);
semtree_node_t *semtreeFindChildByName(semtree_node_t *parent, String *name);
semtree_node_t *codegenTakeCodegenChild(semtree_node_t *parent);
void semtreeForEachSymbol(const semtree_node_t *scope, semtreeSymbolIterCB cb, void *userData);


#endif 
// SEMTREE_H