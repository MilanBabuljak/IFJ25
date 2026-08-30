/***************************************************
 * Project: Implementation of the IFJ25 language compiler
 * File   : codegen.c
 * Author : Milan Babuljak (xbabulm00)
 *        : Jaroslav Synek (xsynekj00)
 * Date   : 2. 12. 2025
 *
 * Description: Header file for code generation implementation
 ***************************************************/

#ifndef CODEGEN_H
#define CODEGEN_H

#include "ast.h"
#include "semtree.h"


void codegenPrint(Code* ast, semtree_node_t* globalScope);
void codegenEmitEscapedStringLiteral(String* s);
void codegenDeclarGlobalVars(semtree_node_t* scope);

void codegenPrintExpr(Expr* expr, int indent, semtree_node_t *scope, bool* isFloatOperation);

void codegenExprPlusExpr(ExprOpExpr* exprOpExpr, semtree_node_t *scope, bool* isFloatOperation);
void codegenExprMinusExpr(ExprOpExpr* exprOpExpr, semtree_node_t *scope, bool* isFloatOperation);
void codegenExprMulExpr(ExprOpExpr* exprOpExpr, semtree_node_t *scope, bool* isFloatOperation);
void codegenExprDivExpr(ExprOpExpr* exprOpExpr, semtree_node_t *scope, bool* isFloatOperation);

void codegenCompGtExpr(ExprOpExpr* exprOpExpr, semtree_node_t *scope);
void codegenCompLtExpr(ExprOpExpr* exprOpExpr, semtree_node_t *scope);
void codegenCompEqExpr(ExprOpExpr* exprOpExpr, semtree_node_t *scope);
void codegenCompLteExpr(ExprOpExpr* exprOpExpr, semtree_node_t *scope);
void codegenCompGteExpr(ExprOpExpr* exprOpExpr, semtree_node_t *scope);
void codegenCompNeqExpr(ExprOpExpr* exprOpExpr, semtree_node_t *scope);


void codegenPrintBody(Body* ast, int indent, semtree_node_t *scope);
void codegenPringIfStm(void* cmd, int indent, semtree_node_t *parentScope, semtree_node_t *ifScope, semtree_node_t *elseScope);
void codegenPrintRet(void* cmd, int indent, semtree_node_t *scope);
void codegenPrintVarDecl(void* cmd, int indent, semtree_node_t *scope);

const char *codegenLookupLabel(semtree_node_t *scope, String *id);
semtree_node_t *codegenAcquireChildScope(semtree_node_t *scope, const char *context);
void codegenDeclareLocalVars(semtree_node_t* scope);


#endif // CODEGEN_H