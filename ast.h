/***************************************************
 * Project: Implementation of the IFJ25 language compiler
 * File   : ast.h
 * Author : Jaroslav Synek (xsynekj00)
 * Date   : 2. 12. 2025
 *
 * Description: Header file for abstract syntax tree structures
 ***************************************************/
    

#ifndef AST_H
#define AST_H

#include <stdio.h>

#include "string.h"
#include "lexer.h"

#define None -1
#define MAX_PARAMS_COUNT 32

typedef enum {
    funcDecl,
    getter,
    setter
} GlobCmdType;

typedef enum {
    ifStm,
    ret,
    varDecl,
    whileLoop,
    assign,
    block,
    funcCall,
    builtinCall
} CmdType;

typedef enum {
    funcCallExpr,
    builtinCallExpr,
    idExpr,
    numExpr,
    strExpr,
    trueExpr,
    falseExpr,
    nullExpr,
    innerExpr,
    exprOpExpr,
    isExpr
} ExprType;

typedef enum {
    addExpr,
    subExpr,
    mulExpr,
    divExpr,
    gtExpr,
    ltExpr,
    geExpr,
    leExpr,
    eqExpr,
    neqExpr
} ExprOpExprType;

typedef enum {
    isString,
    isNum,
    isNull
} IsExprType;

typedef struct Code_T {
    GlobCmdType type;

    void* globCmd;
    struct Code_T* nextGlobalCmd;
} Code;

typedef struct Expr_T {
    ExprType type;

    void* expr;
} Expr;

typedef struct { 
    String* id;   
} FuncCallExpr;

typedef struct { 
    String* id;   
} BuiltinCallExpr;

typedef struct { 
    String* id;   
} IdExpr;

typedef struct { 
    String* id;   
} NumExpr;

typedef struct { 
    String* id;   
} StrExpr;

typedef struct { 
    char _unused;
} TrueExpr;

typedef struct { 
    char _unused;
} FalseExpr;

typedef struct { 
    char _unused;
} NullExpr;

typedef struct { 
    Expr* expr;   
} InnerExpr;

typedef struct { 
    ExprOpExprType type;
    
    Expr* expr1;
    Expr* expr2;
} ExprOpExpr;

typedef struct { 
    Expr* testedExpr;
    IsExprType type;  
} IsExpr;

typedef struct Body_T {
    CmdType type;
    
    void* cmd;
    struct Body_T* nextCmd;
} Body;

typedef struct {
    Expr* cond;
    Body* ifBody;
    Body* elseBody;
} IfStm;

typedef struct {
    Expr* expr;
} Ret;

typedef struct {
    String* id;
    Expr* expr;
} VarDecl;

typedef struct {
    Expr* expr;
    Body* body;
} While;

typedef struct {
    String* id;
    Expr* expr;
} Assign;

typedef struct {
    String* id;
    Expr** args;
    int argsCount;
} FuncCall;

typedef struct {
    String* id;
    Expr** args;
    int argsCount;
} BuiltinCall;

typedef struct {
    String* id;
    String** params;
    int paramsCount;
    Body* body;
} FuncDecl;

typedef struct {
    String* id;
    Body* body;
} Getter;

typedef struct {
    String* id;
    String* param;
    Body* body;
} Setter;

Code* getAST(FILE* file);

#endif // AST_H