/***************************************************
 * Project: Implementation of the IFJ25 language compiler
 * File   : semtree.c
 * Author : Milan Babuljak (xbabulm00)
 *        : Jaroslav Synek (xsynekj00)   
 * Date   : 2. 12. 2025
 *
 * Description: Implementation of semantic tree (scope tree) 
 ***************************************************/

#include "semtree.h"
#include "symtable.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "error.h"


static size_t gSemtreeScopeCounter = 0;

static char *semtreeStrDup(const char *src) {
    if (!src) return NULL;
    size_t len = strlen(src) + 1;
    char *dst = malloc(len);
    if (dst == NULL) {
        fprintf(stderr, "Internal error: Memory allocation failed.\n");
        exit(INTERNAL_ERROR);
        }
    if (!dst) return NULL;
    memcpy(dst, src, len);
    return dst;
}

// Semtree = scopes
// symTable = symbols in scope
semtree_node_t *semtreeCreateNode(String *name) {
	semtree_node_t *n = malloc(sizeof(*n));
	if (!n) {
        fprintf(stderr, "Internal error: Memory allocation failed.\n");
        exit(INTERNAL_ERROR);
    }
	n->name = name;
	n->parent = NULL;
    n->children = NULL;
    n->childCount = 0;
    n->childIter = 0;
    n->scopeId = gSemtreeScopeCounter++;
    n->indexInParent = 0;
    n->prefix = NULL;
	return n;
}

semtree_node_t *semtreeCreateChild(semtree_node_t *parent, String *name) {
    if (!parent) return NULL;
    semtree_node_t *child = semtreeCreateNode(name);
    if (!child) return NULL;
    if (semtreeAddChild(parent, child) != 0) {
        free(child);
        return NULL;
    }
    return child;
}

int semtreeAddsymName(semtree_node_t *node, String *symName) {
    if (!node || !symName) return -1;

    Scope scope_type = node->parent ? LOCAL : GLOBAL;
    if (Insert(&node->symTab, symName, DTYPE_NULL, scope_type) != 0) {
        return -1;
    }
	return 0;
}

int semtreeAddChild(semtree_node_t *parent, semtree_node_t *child) {
    if (!parent || !child) return -1;
    semtree_node_t **tmp = realloc(parent->children, (parent->childCount + 1) * sizeof(*tmp));
    if (!tmp) return -1;
    parent->children = tmp;
    parent->children[parent->childCount++] = child;
    child->parent = parent;
    child->indexInParent = parent->childCount - 1;
    return 0;
}

bool semTreeSymbolDefinedHere(const semtree_node_t *node, String *symName) {
    if (!node || !symName) return false;
    return search(node->symTab, symName) != NULL;
}


// Looking for symbol in current and all parent scopes
bool semtreeResolveSymbol(const semtree_node_t *node, String *symName) {
    const semtree_node_t *current = node;
    while (current) {
        if (semTreeSymbolDefinedHere(current, symName)) {
            return true;
        }
        current = current->parent;
    }
    return false;
}

void semtreeFree(semtree_node_t *node) {
    if (!node) return;
    for (size_t i = 0; i < node->childCount; ++i) {
        semtreeFree(node->children[i]);
    }
    free(node->children);
    FreeTree(node->symTab);
    free(node->prefix);
    free(node);
}

void bottomTopPrint(semtree_node_t *node) {
    if (!node) return;
    if (node->parent) {
        bottomTopPrint(node->parent);
    }
    const char *label = node->name ? stringGetCStr(node->name) : "<scope>";
    printf("%s\n", label);
}

void semtreeResetCodegenCursors(semtree_node_t *node) {
    if (!node) return;
    node->childIter = 0;
    for (size_t i = 0; i < node->childCount; ++i) {
        semtreeResetCodegenCursors(node->children[i]);
    }
}

static int semtreeAssignLabels(Node *symRoot, const char *prefix, bool isGlobal) {
    if (!symRoot) return 0;
    if (semtreeAssignLabels(symRoot->LChild, prefix, isGlobal) != 0) {
        return -1;
    }

    const char *sym = symRoot->symName ? stringGetCStr(symRoot->symName) : "__anon";
    const char *frame = isGlobal ? "GF" : "LF";

    size_t needed = strlen(frame) + strlen("@__") + strlen(prefix) + 1 + strlen(sym) + 1;
    char *label = malloc(needed);
    if (label == NULL) {
        fprintf(stderr, "Internal error: Memory allocation failed.\n");
        exit(INTERNAL_ERROR);
    }
    snprintf(label, needed, "%s@__%s_%s", frame, prefix, sym);
    free(symRoot->label);
    symRoot->label = label;

    if (semtreeAssignLabels(symRoot->RChild, prefix, isGlobal) != 0) {
        return -1;
    }
    return 0;
}

static int semtreeAssignPrefix(semtree_node_t *node, const char *parentPrefix) {
    if (!node) return 0;
    if (!parentPrefix) parentPrefix = "global";

    if (!node->parent) {
        node->prefix = semtreeStrDup("global");
    } else if (node->parent && node->parent->parent == NULL && node->name) {
        const char *base = stringGetCStr(node->name);
        node->prefix = semtreeStrDup(base);
    } else if (node->name) {
        const char *base = stringGetCStr(node->name);
        size_t needed = strlen(parentPrefix) + 1 + strlen(base) + 1;
        node->prefix = malloc(needed);
        if (node->prefix == NULL) {
            fprintf(stderr, "Internal error: Memory allocation failed.\n");
            exit(INTERNAL_ERROR);
        }
        snprintf(node->prefix, needed, "%s_%s", parentPrefix, base);
    } else {
        char buffer[64];
        snprintf(buffer, sizeof(buffer), "%sScope%zu", parentPrefix, node->indexInParent);
        node->prefix = semtreeStrDup(buffer);
    }
    if (!node->prefix) return -1;

    bool isGlobal = (node->parent == NULL);
    if (semtreeAssignLabels(node->symTab, node->prefix, isGlobal) != 0) {
        return -1;
    }

    for (size_t i = 0; i < node->childCount; ++i) {
        if (semtreeAssignPrefix(node->children[i], node->prefix) != 0) {
            return -1;
        }
    }
    return 0;
}

// Prepare labels for all symbols in the scope tree for codegen - assigning prefixes in early stage
int semtreePrepareCodegenLabels(semtree_node_t *node) {
    if (!node) return -1;
    return semtreeAssignPrefix(node, "global");
}

// Getting label for symbol in scope tree 
const char *semtreeLookupLabel(const semtree_node_t *node, String *symName) {
    const semtree_node_t *current = node;
    while (current) {
        Node *found = search(current->symTab, symName);
        if (found && found->label) {
            return found->label;
        }
        current = current->parent;
    }
    return NULL;
}

semtree_node_t *semtreeFindChildByName(semtree_node_t *parent, String *name) {
    if (!parent || !name) return NULL;
    for (size_t i = 0; i < parent->childCount; ++i) {
        if (parent->children[i]->name && stringCompare(parent->children[i]->name, name) == 0) {
            return parent->children[i];
        }
    }
    return NULL;
}

semtree_node_t *codegenTakeCodegenChild(semtree_node_t *parent) {
    if (!parent) return NULL;
    if (parent->childIter >= parent->childCount) {
        return NULL;
    }
    return parent->children[parent->childIter++];
}

static void semtreeForEachSymbolInner(Node *root, semtreeSymbolIterCB cb, void *userData) {
    if (!root || !cb) return;
    semtreeForEachSymbolInner(root->LChild, cb, userData);
    cb(root->symName, root->label, userData);
    semtreeForEachSymbolInner(root->RChild, cb, userData);
}

void semtreeForEachSymbol(const semtree_node_t *scope, semtreeSymbolIterCB cb, void *userData) {
    if (!scope || !cb) return;
    semtreeForEachSymbolInner(scope->symTab, cb, userData);
}