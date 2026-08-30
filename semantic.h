/***************************************************
 * Project: Implementation of the IFJ25 language compiler
 * File   : semantic.h
 * Author : Adam Bisa (xbisaad00)
 * Date   : 2. 12. 2025
 *
 * Description: Header file for semantic analysis
 ***************************************************/


#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "ast.h"
#include "error.h"
#include "semtree.h"

// Argument type expectations for builtins
typedef enum {
    ARG_ANY,    // any type allowed
    ARG_NUM,    // must be numeric literal
    ARG_STR,    // must be string literal
} ArgType;

typedef struct {
    const char *name;
    int minArgs;
    int maxArgs; // -1 == unlimited
    ArgType argTypes[3]; // max 3 args for type checking
} BuiltinSpec;

semtree_node_t *semanticAnalyze(Code *ast, ErrorCode *outErr);

extern const BuiltinSpec builtinSpecs[];

typedef struct {
    String *name;
    int paramsCount;
    GlobCmdType kind;
} SemFuncEntry;

typedef struct {
    SemFuncEntry *items;
    size_t count;
    size_t capacity;
} SemFuncRegistry;

typedef struct {
    Code *ast;
    SemFuncRegistry registry;
    semtree_node_t *globalScope;
} SemanticContext;

#endif // SEMANTIC_H
