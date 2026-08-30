/***************************************************
 * Project: Implementation of the IFJ25 language compiler
 * File   : semantic.c
 * Author : Adam Bisa (xbisaad00)
 *        : Jaroslav Synek (xsynekj00)   
 * Date   : 2. 12. 2025
 *
 * Description: Implementation of semantic analysis 
 ***************************************************/



#include "semantic.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "semtree.h"

const BuiltinSpec builtinSpecs[] = {
    {"readString", 0, 0, {ARG_ANY, ARG_ANY, ARG_ANY}},
    {"readNum", 0, 0, {ARG_ANY, ARG_ANY, ARG_ANY}},
    {"readBool", 0, 0, {ARG_ANY, ARG_ANY, ARG_ANY}},
    {"read_str", 0, 0, {ARG_ANY, ARG_ANY, ARG_ANY}},
    {"read_num", 0, 0, {ARG_ANY, ARG_ANY, ARG_ANY}},
    {"read_bool", 0, 0, {ARG_ANY, ARG_ANY, ARG_ANY}},
    {"write", 1, -1, {ARG_ANY, ARG_ANY, ARG_ANY}},
    {"floor", 1, 1, {ARG_NUM, ARG_ANY, ARG_ANY}},
    {"str", 1, 1, {ARG_ANY, ARG_ANY, ARG_ANY}},
    {"len", 1, 1, {ARG_STR, ARG_ANY, ARG_ANY}},
    {"length", 1, 1, {ARG_STR, ARG_ANY, ARG_ANY}},
    {"chr", 1, 1, {ARG_NUM, ARG_ANY, ARG_ANY}},
    {"ord", 2, 2, {ARG_STR, ARG_NUM, ARG_ANY}},
    {"substring", 3, 3, {ARG_STR, ARG_NUM, ARG_NUM}},
    {"strcmp", 2, 2, {ARG_STR, ARG_STR, ARG_ANY}},
};

gettersAndSetters gas = {.getters = NULL, .setters = NULL};

 void semRegistryInit(SemFuncRegistry *reg) {
    reg->items = NULL;
    reg->count = 0;
    reg->capacity = 0;
}

 void semRegistryDestroy(SemFuncRegistry *reg) {
    free(reg->items);
    reg->items = NULL;
    reg->count = 0;
    reg->capacity = 0;
}

 int semRegistryEnsureCapacity(SemFuncRegistry *reg) {
    if (reg->count < reg->capacity) return 0;
    size_t newCap = reg->capacity == 0 ? 8 : reg->capacity * 2;
    SemFuncEntry *tmp = realloc(reg->items, newCap * sizeof(*tmp));
    if (!tmp) return -1;
    reg->items = tmp;
    reg->capacity = newCap;
    return 0;
}

 SemFuncEntry *semRegistryFindExact(SemFuncRegistry *reg, String *name, int paramsCount, GlobCmdType kind) {
    for (size_t i = 0; i < reg->count; ++i) {
        SemFuncEntry *entry = &reg->items[i];
        if (entry->kind != kind) continue;
        if (stringCompare(entry->name, name) == 0 && entry->paramsCount == paramsCount) {
            return entry;
        }
    }
    return NULL;
}

 bool semRegistryNameKindExists(SemFuncRegistry *reg, String *name, GlobCmdType kind) {
    for (size_t i = 0; i < reg->count; ++i) {
        SemFuncEntry *entry = &reg->items[i];
        if (entry->kind == kind && stringCompare(entry->name, name) == 0) {
            return true;
        }
    }
    return false;
}

 ErrorCode semRegistryAdd(SemFuncRegistry *reg, String *name, int paramsCount, GlobCmdType kind) {
    if (kind == funcDecl) {
        if (semRegistryFindExact(reg, name, paramsCount, kind)) {
            fprintf(stderr, "Semantic error: function '%s' redefined with identical arity.\n", stringGetCStr(name));
            return SEMANTIC_ERROR_REDEFINITION;
        }
    } else {
        if (semRegistryNameKindExists(reg, name, kind)) {
            fprintf(stderr, "Semantic error: %s '%s' redefined.\n",
                kind == getter ? "getter" : "setter",
                stringGetCStr(name));
            return SEMANTIC_ERROR_REDEFINITION;
        }

        if (kind == getter) {
            getterListItem* g = gas.getters;
            if (g == NULL) {
                g = malloc(sizeof(getterListItem));
                g->id = name;
                g->nextGetter = NULL;
                gas.getters = g;
                return SUCCESS;
            }
            if (strcmp(g->id->str, name->str) == 0) {
                fprintf(stderr, "Semantic error: redefinition of getter.\n");
                return SEMANTIC_ERROR_REDEFINITION;
            }
            while (g->nextGetter != NULL) {
                if (strcmp(g->id->str, name->str) == 0) {
                    fprintf(stderr, "Semantic error: redefinition of getter.\n");
                    return SEMANTIC_ERROR_REDEFINITION;
                }
                
                g = g->nextGetter;
            }
            getterListItem* gNew = malloc(sizeof(getterListItem));
            gNew->id = name;
            gNew->nextGetter = NULL;
            g->nextGetter = gNew;
            return SUCCESS;
        } else {
            setterListItem* s = gas.setters;
            if (s == NULL) {
                s = malloc(sizeof(setterListItem));
                s->id = name;
                s->nextSetter = NULL;
                gas.setters = s;
                return SUCCESS;
            }
            if (strcmp(s->id->str, name->str) == 0) {
                fprintf(stderr, "Semantic error: redefinition of setter.\n");
                return SEMANTIC_ERROR_REDEFINITION;
            }
            while (s->nextSetter != NULL) {
                if (strcmp(s->id->str, name->str) == 0) {
                    fprintf(stderr, "Semantic error: redefinition of setter.\n");
                    return SEMANTIC_ERROR_REDEFINITION;
                }
                
                s = s->nextSetter;
            }
            setterListItem* sNew = malloc(sizeof(setterListItem));
            sNew->id = name;
            sNew->nextSetter = NULL;
            s->nextSetter = sNew;
            return SUCCESS;
        }
    }
    if (semRegistryEnsureCapacity(reg) != 0) {
        return INTERNAL_ERROR;
    }
    SemFuncEntry *entry = &reg->items[reg->count++];
    entry->name = name;
    entry->paramsCount = paramsCount;
    entry->kind = kind;
    return SUCCESS;
}

 bool semRegistryHasMainZeroSafe(SemFuncRegistry *reg) {
    for (size_t i = 0; i < reg->count; ++i) {
        SemFuncEntry *entry = &reg->items[i];
        if (entry->kind != funcDecl) continue;
        const char *name = stringGetCStr(entry->name);
        if (strcmp(name, "main") == 0 && entry->paramsCount == 0) {
            return true;
        }
    }
    return false;
}

 ErrorCode semanticValidateUniqueParams(String **params, int count) {
    if (!params || count <= 1) return SUCCESS;
    for (int i = 0; i < count; ++i) {
        for (int j = i + 1; j < count; ++j) {
            if (stringCompare(params[i], params[j]) == 0) {
                fprintf(stderr, "Semantic error: parameter '%s' redeclared in signature.\n", stringGetCStr(params[i]));
                return SEMANTIC_ERROR_REDEFINITION;
            }
        }
    }
    return SUCCESS;
}

 ErrorCode semanticSeedParams(semtree_node_t *scope, String **params, int count) {
    if (!scope || !params) return SUCCESS;
    for (int i = 0; i < count; ++i) {
        if (!params[i]) continue;
        if (semTreeSymbolDefinedHere(scope, params[i])) {
            fprintf(stderr, "Semantic error: parameter '%s' redeclared inside scope.\n", stringGetCStr(params[i]));
            return SEMANTIC_ERROR_REDEFINITION;
        }
        if (semtreeAddsymName(scope, params[i]) != 0) {
            return INTERNAL_ERROR;
        }
    }
    return SUCCESS;
}

// Check if identifier is a global variable (starts with __)
 bool isGlobalVariable(String *id) {
    if (!id) return false;
    const char *name = stringGetCStr(id);
    return name && name[0] == '_' && name[1] == '_';
}

 ErrorCode semanticRequireSymbol(semtree_node_t *scope, String *id, GlobCmdType kind) {
    if (!id) return SUCCESS;
    if (isGlobalVariable(id)) return SUCCESS;
    if (kind == getter) {
        getterListItem* g = gas.getters;
        while (g != NULL) {
            if (strcmp(g->id->str, id->str) == 0) return SUCCESS;
            g = g->nextGetter;
        }
    } else {
        setterListItem* s = gas.setters;
        while (s != NULL) {
            if (strcmp(s->id->str, id->str) == 0) return SUCCESS;
            s = s->nextSetter;
        }
    }
    if (!semtreeResolveSymbol(scope, id)) {
        fprintf(stderr, "Semantic error: symbol '%s' is not defined in current scope.\n", stringGetCStr(id));
        return SEMANTIC_ERROR_UNDEFINED;
    }
    return SUCCESS;
}

 ErrorCode semanticCheckExpr(SemanticContext *ctx, Expr *expr, semtree_node_t *scope);
 ErrorCode semanticCheckBody(SemanticContext *ctx, Body *body, semtree_node_t *scope);

 ErrorCode semanticCheckFuncCall(SemanticContext *ctx, FuncCall *call, semtree_node_t *scope) {
    if (!call) return SUCCESS;
    if (call->args) {
        for (int i = 0; i < call->argsCount; ++i) {
            ErrorCode err = semanticCheckExpr(ctx, call->args[i], scope);
            if (err != SUCCESS) return err;
        }
    }
    bool nameExists = false;
    for (size_t i = 0; i < ctx->registry.count; ++i) {
        SemFuncEntry *entry = &ctx->registry.items[i];
        if (entry->kind != funcDecl) continue;
        if (stringCompare(entry->name, call->id) == 0) {
            nameExists = true;
            if (entry->paramsCount == call->argsCount) {
                return SUCCESS;
            }
        }
    }
    if (!nameExists) {
        // TODO: Fixme Check for built-in functions, fornow it's just check if its "write"
        
        String *writeStr = makeS("write");
        int cmp = stringCompare(call->id, writeStr);
        if (cmp == 0) {
            return SUCCESS;
        }

        // TODO: HLUPE RIESENIE NA TESTOVANIE, ODSTRANIT!!!! HAMBIM SA ZA TO
         String *writeStr2 = makeS("read_str");
        int cmp2 = stringCompare(call->id, writeStr2);
        if (cmp2 == 0) {
            return SUCCESS;
        }

        fprintf(stderr, "Semantic error: function '%s' is not defined.\n", stringGetCStr(call->id));
        return SEMANTIC_ERROR_UNDEFINED;
    }
    fprintf(stderr, "Semantic error: function '%s' called with wrong number of arguments.\n", stringGetCStr(call->id));
    return SEMANTIC_ERROR_FUNCTION_CALL;
}

 const BuiltinSpec *semanticFindBuiltin(String *id) {
    if (!id) return NULL;
    const char *needle = stringGetCStr(id);
    for (size_t i = 0; i < sizeof(builtinSpecs) / sizeof(builtinSpecs[0]); ++i) {
        if (strcmp(needle, builtinSpecs[i].name) == 0) {
            return &builtinSpecs[i];
        }
    }
    return NULL;
}
// Check if expression type is definitely incompatible with expected type
// Returns true if there's a definite type mismatch (literal type conflicts)
 bool exprTypeConflicts(Expr *expr, ArgType expected) {
    if (!expr || expected == ARG_ANY) return false;
    
    // Only check literals - variables/calls could be any type at runtime
    switch (expr->type) {
        case numExpr:
            // num literal passed where string expected
            return (expected == ARG_STR);
        case strExpr:
            // string literal passed where num expected
            return (expected == ARG_NUM);
        case trueExpr:
        case falseExpr:
            // bool literal passed where num or string expected
            return (expected == ARG_NUM || expected == ARG_STR);
        case nullExpr:
            // null is generally compatible (nullable types)
            return false;
        default:
            // Variables, function calls, expressions - can't determine at compile time
            return false;
    }
}

 ErrorCode semanticCheckBuiltinCall(SemanticContext *ctx, BuiltinCall *call, semtree_node_t *scope) {
    if (!call) return SUCCESS;
    if (call->args) {
        for (int i = 0; i < call->argsCount; ++i) {
            ErrorCode err = semanticCheckExpr(ctx, call->args[i], scope);
            if (err != SUCCESS) return err;
        }
    }
    const BuiltinSpec *spec = semanticFindBuiltin(call->id);
    if (!spec) {
        fprintf(stderr, "Semantic error: builtin Ifj.%s is not defined.\n", stringGetCStr(call->id));
        return SEMANTIC_ERROR_UNDEFINED;
    }
    if (call->argsCount < spec->minArgs || (spec->maxArgs >= 0 && call->argsCount > spec->maxArgs)) {
        fprintf(stderr, "Semantic error: builtin Ifj.%s called with invalid arity.\n", stringGetCStr(call->id));
        return SEMANTIC_ERROR_FUNCTION_CALL;
    }
    
    // Type check arguments for builtins with specific type requirements
    if (call->args) {
        int checkCount = call->argsCount < 3 ? call->argsCount : 3;
        for (int i = 0; i < checkCount; ++i) {
            if (exprTypeConflicts(call->args[i], spec->argTypes[i])) {
                const char *expected = (spec->argTypes[i] == ARG_NUM) ? "Num" : "String";
                fprintf(stderr, "Semantic error: builtin Ifj.%s argument %d expects %s.\n", 
                        stringGetCStr(call->id), i + 1, expected);
                return SEMANTIC_ERROR_FUNCTION_CALL;
            }
        }
    }
    
    return SUCCESS;
}

 ErrorCode semanticCheckExpr(SemanticContext *ctx, Expr *expr, semtree_node_t *scope) {
    (void)ctx;
    if (!expr) return SUCCESS;
    switch (expr->type) {
        case idExpr: {
            IdExpr *id = expr->expr;
            if (!id) return SUCCESS;
            return semanticRequireSymbol(scope, id->id, getter);
        }
        case innerExpr: {
            InnerExpr *inner = expr->expr;
            return semanticCheckExpr(ctx, inner ? inner->expr : NULL, scope);
        }
        case exprOpExpr: {
            ExprOpExpr *op = expr->expr;
            if (!op) return SUCCESS;
            ErrorCode err = semanticCheckExpr(ctx, op->expr1, scope);
            if (err != SUCCESS) return err;
            return semanticCheckExpr(ctx, op->expr2, scope);
        }
        case funcCallExpr: {
            // FuncCall stored in expr for function call expressions
            FuncCall *call = expr->expr;
            return semanticCheckFuncCall(ctx, call, scope);
        }
        case builtinCallExpr: {
            // BuiltinCall stored in expr for builtin call expressions
            BuiltinCall *call = expr->expr;
            return semanticCheckBuiltinCall(ctx, call, scope);
        }
        case numExpr:
        case strExpr:
        case trueExpr:
        case falseExpr:
        case nullExpr:
        case isExpr:
            return SUCCESS;
    }
    return SUCCESS;
}

 ErrorCode semanticCheckCommand(SemanticContext *ctx, Body *cmd, semtree_node_t *scope) {
    if (!cmd) return SUCCESS;
    ErrorCode err = SUCCESS;
    switch (cmd->type) {
        case varDecl: {
            VarDecl *decl = cmd->cmd;
            if (!decl) break;
            if (decl->expr) {
                err = semanticCheckExpr(ctx, decl->expr, scope);
                if (err != SUCCESS) return err;
            }
            if (semTreeSymbolDefinedHere(scope, decl->id)) {
                fprintf(stderr, "Semantic error: variable '%s' redeclared in the same scope.\n", stringGetCStr(decl->id));
                return SEMANTIC_ERROR_REDEFINITION;
            }
            if (semtreeAddsymName(scope, decl->id) != 0) {
                return INTERNAL_ERROR;
            }
            break;
        }
        case assign: {
            Assign *assignCmd = cmd->cmd;
            if (!assignCmd) break;
            // Global variables (starting with __) are auto-declared on first use
            if (isGlobalVariable(assignCmd->id)) {
                // Add to global scope if not already defined
                if (!semtreeResolveSymbol(ctx->globalScope, assignCmd->id)) {
                    if (semtreeAddsymName(ctx->globalScope, assignCmd->id) != 0) {
                        return INTERNAL_ERROR;
                    }
                }
            } else {
                err = semanticRequireSymbol(scope, assignCmd->id, setter);
                if (err != SUCCESS) return err;
            }
            return semanticCheckExpr(ctx, assignCmd->expr, scope);
        }
        case ret: {
            Ret *retCmd = cmd->cmd;
            if (!retCmd) break;
            return semanticCheckExpr(ctx, retCmd->expr, scope);
        }
        case ifStm: {
            IfStm *ifCmd = cmd->cmd;
            if (!ifCmd) break;
            err = semanticCheckExpr(ctx, ifCmd->cond, scope);
            if (err != SUCCESS) return err;
            if (ifCmd->ifBody) {
                semtree_node_t *ifScope = semtreeCreateChild(scope, NULL);
                if (!ifScope) return INTERNAL_ERROR;
                err = semanticCheckBody(ctx, ifCmd->ifBody, ifScope);
                if (err != SUCCESS) return err;
            }
            if (ifCmd->elseBody) {
                semtree_node_t *elseScope = semtreeCreateChild(scope, NULL);
                if (!elseScope) return INTERNAL_ERROR;
                err = semanticCheckBody(ctx, ifCmd->elseBody, elseScope);
                if (err != SUCCESS) return err;
            }
            break;
        }
        case whileLoop: {
            While *whileCmd = cmd->cmd;
            if (!whileCmd) break;
            err = semanticCheckExpr(ctx, whileCmd->expr, scope);
            if (err != SUCCESS) return err;
            if (whileCmd->body) {
                semtree_node_t *loopScope = semtreeCreateChild(scope, NULL);
                if (!loopScope) return INTERNAL_ERROR;
                err = semanticCheckBody(ctx, whileCmd->body, loopScope);
                if (err != SUCCESS) return err;
            }
            break;
        }
        case block: {
            Body *blockBody = cmd->cmd;
            if (!blockBody) break;
            semtree_node_t *blockScope = semtreeCreateChild(scope, NULL);
            if (!blockScope) return INTERNAL_ERROR;
            return semanticCheckBody(ctx, blockBody, blockScope);
        }
        case funcCall:
            return semanticCheckFuncCall(ctx, cmd->cmd, scope);
        case builtinCall:
            return semanticCheckBuiltinCall(ctx, cmd->cmd, scope);
    }
    return SUCCESS;
}

 ErrorCode semanticCheckBody(SemanticContext *ctx, Body *body, semtree_node_t *scope) {
    if (!body) return SUCCESS;
    for (Body *cmd = body; cmd != NULL; cmd = cmd->nextCmd) {
        ErrorCode err = semanticCheckCommand(ctx, cmd, scope);
        if (err != SUCCESS) return err;
    }
    return SUCCESS;
}

 ErrorCode semanticCheckFunctionBody(SemanticContext *ctx, FuncDecl *func) {
    if (!func) return SUCCESS;
    semtree_node_t *scope = semtreeCreateChild(ctx->globalScope, func->id);
    if (!scope) return INTERNAL_ERROR;
    ErrorCode err = semanticSeedParams(scope, func->params, func->paramsCount);
    if (err != SUCCESS) return err;
    return semanticCheckBody(ctx, func->body, scope);
}

 ErrorCode semanticCheckGetterBody(SemanticContext *ctx, Getter *getterDecl) {
    if (!getterDecl) return SUCCESS;
    semtree_node_t *scope = semtreeCreateChild(ctx->globalScope, getterDecl->id);
    if (!scope) return INTERNAL_ERROR;
    return semanticCheckBody(ctx, getterDecl->body, scope);
}

 ErrorCode semanticCheckSetterBody(SemanticContext *ctx, Setter *setterDecl) {
    if (!setterDecl) return SUCCESS;
    semtree_node_t *scope = semtreeCreateChild(ctx->globalScope, setterDecl->id);
    if (!scope) return INTERNAL_ERROR;
    if (setterDecl->param) {
        if (semtreeAddsymName(scope, setterDecl->param) != 0) {
            return INTERNAL_ERROR;
        }
    }
    return semanticCheckBody(ctx, setterDecl->body, scope);
}

 ErrorCode semanticRegisterGlobals(SemanticContext *ctx) {
    for (Code *node = ctx->ast; node != NULL; node = node->nextGlobalCmd) {
        switch (node->type) {
            case funcDecl: {
                FuncDecl *func = node->globCmd;
                if (!func) continue;
                ErrorCode err = semanticValidateUniqueParams(func->params, func->paramsCount);
                if (err != SUCCESS) return err;
                err = semRegistryAdd(&ctx->registry, func->id, func->paramsCount, funcDecl);
                if (err != SUCCESS) return err;
                break;
            }
            case getter: {
                Getter *g = node->globCmd;
                if (!g) continue;
                ErrorCode err = semRegistryAdd(&ctx->registry, g->id, 0, getter);
                if (err != SUCCESS) return err;
                break;
            }
            case setter: {
                Setter *s = node->globCmd;
                if (!s) continue;
                if (s->param == NULL) {
                    fprintf(stderr, "Semantic error: setter '%s' missing parameter.\n", stringGetCStr(s->id));
                    return SEMANTIC_ERROR_REDEFINITION;
                }
                ErrorCode err = semRegistryAdd(&ctx->registry, s->id, 1, setter);
                if (err != SUCCESS) return err;
                break;
            }
        }
    }
    if (!semRegistryHasMainZeroSafe(&ctx->registry)) {
        fprintf(stderr, "Semantic error: function main() with zero parameters must be defined.\n");
        return SEMANTIC_ERROR_UNDEFINED;
    }
    return SUCCESS;
}

 ErrorCode semanticCheckGlobals(SemanticContext *ctx) {
    for (Code *node = ctx->ast; node != NULL; node = node->nextGlobalCmd) {
        switch (node->type) {
            case funcDecl: {
                ErrorCode err = semanticCheckFunctionBody(ctx, node->globCmd);
                if (err != SUCCESS) return err;
                break;
            }
            case getter: {
                ErrorCode err = semanticCheckGetterBody(ctx, node->globCmd);
                if (err != SUCCESS) return err;
                break;
            }
            case setter: {
                ErrorCode err = semanticCheckSetterBody(ctx, node->globCmd);
                if (err != SUCCESS) return err;
                break;
            }
        }
    }
    return SUCCESS;
}

semtree_node_t *semanticAnalyze(Code *ast, ErrorCode *outErr) {
    if (outErr) {
        *outErr = SUCCESS;
    }
    if (!ast) {
        return NULL;
    }
    SemanticContext ctx = {
        .ast = ast,
    };
    semRegistryInit(&ctx.registry);
    ctx.globalScope = semtreeCreateNode(NULL);
    if (!ctx.globalScope) {
        semRegistryDestroy(&ctx.registry);
        if (outErr) {
            *outErr = INTERNAL_ERROR;
        }
        return NULL;
    }
    ErrorCode err = semanticRegisterGlobals(&ctx);
    if (err == SUCCESS) {
        err = semanticCheckGlobals(&ctx);
    }
    semRegistryDestroy(&ctx.registry);
    if (err != SUCCESS) {
        semtreeFree(ctx.globalScope);
        if (outErr) {
            *outErr = err;
        }
        return NULL;
    }
    if (outErr) {
        *outErr = SUCCESS;
    }
    ctx.globalScope->getters = gas.getters;
    ctx.globalScope->setters = gas.setters;
    return ctx.globalScope;
}