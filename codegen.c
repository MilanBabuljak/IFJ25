/***************************************************
 * Project: Implementation of the IFJ25 language compiler
 * File   : codegen.c
 * Author : Milan Babuljak (xbabulm00)
 *        : Jaroslav Synek (xsynekj00)
 * Date   : 2. 12. 2025
 *
 * Description: Code generation implementation
 ***************************************************/


#include <stdio.h>
#include "builtin.h"
#include <string.h>
#include "codegen.h"
#include "semtree.h"
#include "error.h"

// Global counters for unique labels
int ifCount = 0;
int addCount = 0;
int mulCount = 0;

// Helper function to determine if a string represents a float
bool isFloat(String* value) {
    for (int i = 0; i < value->length; i++) {
        if (value->str[i] == '.') return true;
    }
    return false;
}

// Helper function to determine whether value is a getter
bool isGetter(String* value, getterListItem* getters) {
    if (getters == NULL) return false;
    if (getters->id == NULL) return false;
    getterListItem* g = getters;
    while (g != NULL) {
        if (strcmp(g->id->str, value->str) == 0) return true;
        g = g->nextGetter;
    }
    return false;
}

// Helper function to determine whether value is a setter
bool isSetter(String* value, setterListItem* setters) {
    if (setters == NULL) return false;
    if (setters->id == NULL) return false;
    setterListItem* s = setters;
    while (s != NULL) {
        if (strcmp(s->id->str, value->str) == 0) return true;
        s = s->nextSetter;
    }
    return false;
}

// Helper function to get global scope because that is where getters and setters are stored
semtree_node_t* getGlobalScope(semtree_node_t* scope) {
    if (scope == NULL) {
        fprintf(stderr, "scope was NULL\n");
        exit(INTERNAL_ERROR);
    }
    while (scope->parent != NULL) {
        scope = scope->parent;
    }
    return scope;
}

// Determine if expression definitely results in a string
bool codegenExprDefString(Expr *expr) {
    if (!expr) {
        return false;
    }
    switch (expr->type) {
        case strExpr:
            return true;
        case innerExpr: {
            InnerExpr *inner = expr->expr;
            return inner && codegenExprDefString(inner->expr);
        }
        case exprOpExpr: {
            ExprOpExpr *op = expr->expr;
            if (!op) {
                return false;
            }

            // Concat
            if (op->type == addExpr) {
                return codegenExprDefString(op->expr1) &&
                       codegenExprDefString(op->expr2);
            }

            // Iter
            if (op->type == mulExpr) {
                return codegenExprDefString(op->expr1);
            }
            return false;
        }
        default:
            return false;
    }
}

// Looking up symbol label in the given scope and its ancestors
 const char *codegenLookupLabel(semtree_node_t *scope, String *id) {
    if (!id) {
        fprintf(stderr, "Codegen error: encountered NULL identifier.\n");
    }
    const char *label = semtreeLookupLabel(scope, id);

    if (!label) {
        fprintf(stderr, "Codegen error: unresolved identifier '%s'.\n", stringGetCStr(id));
    }
    return label;
}

// Acquiring the next child scope for code "generation"
 semtree_node_t *codegenAcquireChildScope(semtree_node_t *scope, const char *context) {
    (void)context;
    semtree_node_t *child = codegenTakeCodegenChild(scope);
    if (!child) {
        return scope;
    }
    return child;
}


// --- IF / ELSE ---
void codegenPringIfStm(void* cmd, int indent, semtree_node_t *parentScope, semtree_node_t *ifScope, semtree_node_t *elseScope) {
    
    ifCount++;

    //Count for nested IFs
    int currentIfCount = ifCount;

    IfStm* cgIfStm = (IfStm*)cmd;

    codegenPrintExpr(cgIfStm->cond, indent + 1, parentScope, false);
    printf("PUSHS bool@true\n");
    printf("JUMPIFEQS $$IF_BODY_%d\n", currentIfCount);
    if (cgIfStm->elseBody) {
        printf("JUMP $$ELSE_BODY_%d\n", currentIfCount);
    } else {
        printf("JUMP $$END_IF_%d\n", currentIfCount);
    }

    // ---- IF BODY ----
    printf("LABEL $$IF_BODY_%d\n", currentIfCount);
    codegenPrintBody(cgIfStm->ifBody, indent + 1, ifScope);
    printf("JUMP $$END_IF_%d\n", currentIfCount);

    // ---- ELSE BODY (if exists) ----
    if (cgIfStm->elseBody) {
        printf("LABEL $$ELSE_BODY_%d\n", currentIfCount);
        codegenPrintBody(cgIfStm->elseBody, indent + 1, elseScope);
    }
    printf("LABEL $$END_IF_%d\n", currentIfCount);
}

// --- RETURN ---
void codegenPrintRet(void* cmd, int indent, semtree_node_t *scope) {
    Ret* cg_ret = (Ret*)cmd;
    codegenPrintExpr(cg_ret->expr, indent + 1, scope, false);
}


// --- VARIABLE DECLARATION ---
void codegenPrintVarDecl(void* cmd, int indent, semtree_node_t *scope) {
    VarDecl* cgVarDecl = (VarDecl*)cmd;
    
    const char *label = codegenLookupLabel(scope, cgVarDecl->id);
    if (cgVarDecl->expr) {
        if (isSetter(cgVarDecl->id, getGlobalScope(scope)->setters)) {
            printf("JUMP $SETTER_VAR_%s\n", cgVarDecl->id->str);    //TODO:
        } else {
            codegenPrintExpr(cgVarDecl->expr, indent + 1, scope, false);
            printf("POPS %s\n", label);
        }
    }
}


// --- WHILE LOOP ---
int whileCount = 0;
void codegenPrintWhileLoop(void* cmd, int indent, semtree_node_t *scope, semtree_node_t *bodyScope) {
    While* cgWhileLopp = (While*)cmd;

    whileCount++;
    int currentWhileCount = whileCount;
    
    printf("LABEL $$WHILE_BODY_%d\n", currentWhileCount);
    codegenPrintExpr(cgWhileLopp->expr, indent + 1, scope, false);
    printf("PUSHS bool@true\n");
    printf("JUMPIFNEQS $$WHILE_BODY_END_%d\n", currentWhileCount);

    codegenPrintBody(cgWhileLopp->body, indent + 1, bodyScope);

    printf("JUMP $$WHILE_BODY_%d\n", currentWhileCount);
    printf("LABEL $$WHILE_BODY_END_%d\n", currentWhileCount);   // Jump back
}
 

// --- ASSIGNMENT ---
void codegenPrintAssign(void* cmd, int indent, semtree_node_t *scope) {
    Assign* cgAssign = (Assign*)cmd;
    if (isSetter(cgAssign->id, getGlobalScope(scope)->setters)) {
        printf("JUMP $SETTER_VAR_%s\n", cgAssign->id->str);    //TODO:
    } else {
        codegenPrintExpr(cgAssign->expr, indent + 1, scope, false);
        const char *label = codegenLookupLabel(scope, cgAssign->id);
        printf("POPS %s\n", label);
    }
}



// --- BLOCK ---
void codegenPrintBlock(void* cmd, int indent, semtree_node_t *scope) {
    Body* cgBlock = (Body*)cmd;
    codegenPrintBody(cgBlock, indent + 1, scope);
}


// --- FUNCTION CALL ---
void codegenPrintFuncCall(void* cmd, int indent, semtree_node_t *scope) {
    FuncCall* cgCall = (FuncCall*)cmd;

    // Argument preparation to stack
    if (cgCall->args) {
        for (int i = cgCall->argsCount - 1; i >= 0; i--) {
            codegenPrintExpr(cgCall->args[i], indent + 1, scope, false);
        }
    }

    printf("CALL $%s$$%i\n", cgCall->id->str, cgCall->argsCount);
}


// --- BUILTIN CALL ---
void codegenPrintBuiltinCall(void* cmd, int indent, semtree_node_t *scope) {
    BuiltinCall* cgCall = (BuiltinCall*)cmd;
    if (!cgCall || !cgCall->id || !cgCall->id->str) {
        fprintf(stderr, "Codegen error: malformed builtin call.\n");
        return;
    }

    const char *buildinName = cgCall->id->str;

    // Ifj.write
    if (strcmp(buildinName, "write") == 0) {
        if (cgCall->args && cgCall->argsCount == 1 ) {
           if (cgCall->args[0]->type == strExpr) {
            StrExpr* sExpr = (StrExpr*)cgCall->args[0]->expr;
            printf("WRITE string@");
            codegenEmitEscapedStringLiteral(sExpr->id);
            printf("\n");
            printf("PUSHS int@0\n");
            return; 
            }

            if (cgCall->args[0]->type == idExpr) {
                IdExpr* idExpr = (IdExpr*)cgCall->args[0]->expr;
                if (isGetter(idExpr->id, getGlobalScope(scope)->getters)) {
                    printf("JUMP $GETTER_VAR_%s\n", idExpr->id->str);    //TODO:
                } else {
                    const char *label = codegenLookupLabel(scope, idExpr->id);
                    printf("WRITE %s\n", label);
                    printf("PUSHS int@0\n");
                }
                return;
            }

        
            if(cgCall->args[0]->type == innerExpr) {
                InnerExpr* innerExpr = (InnerExpr*)cgCall->args[0]->expr;
                if (innerExpr->expr->type == idExpr) {
                    IdExpr* idExpr = (IdExpr*)innerExpr->expr;
                    if (isGetter(idExpr->id, getGlobalScope(scope)->getters)) {
                        printf("JUMP $GETTER_VAR_%s\n", idExpr->id->str);    //TODO:
                    } else {
                        const char *label = codegenLookupLabel(scope, idExpr->id);
                        printf("WRITE %s\n", label);
                        printf("PUSHS int@0\n");
                        return;
                    }
                }
            }

            if(cgCall->args[0]->type == exprOpExpr) {
                ExprOpExpr* exprOp = (ExprOpExpr*)cgCall->args[0]->expr;
                if (exprOp->type == addExpr) {
                    codegenPrintExpr(cgCall->args[0], indent + 1, scope, false);
                    printf("PUSHFRAME\n");
                    printf("CREATEFRAME\n");
                    printf("DEFVAR TF@to_write\n");
                    printf("POPS TF@to_write\n");
                    printf("WRITE TF@to_write\n");
                    printf("CLEARS\n");
                    printf("PUSHS int@0\n");
                    printf("POPFRAME\n");
                    return;
                }


            }

            if (cgCall->args[0]->type == numExpr) {
                NumExpr* numExpr = (NumExpr*)cgCall->args[0]->expr;
                if (isFloat(numExpr->id)) {
                    double value = atof(numExpr->id->str);
                    printf("WRITE float@%a\n", value);
                } else {
                    printf("WRITE int@%s\n", numExpr->id->str);
                }
                printf("PUSHS int@0\n");
                return;
            }

            if (cgCall->args[0]->type == trueExpr) {
                printf("WRITE bool@true\n");
                printf("PUSHS int@0\n");
                return;
            } else if (cgCall->args[0]->type == falseExpr) {
                printf("WRITE bool@false\n");
                printf("PUSHS int@0\n");
                return;
            }

            if (cgCall->args[0]->type == nullExpr) {
                printf("WRITE nil@nil\n");
                printf("PUSHS int@0\n");
                return;
            }


        
        } else {
                    
        }
    }


    // Ifj.read_num

    if (strcmp(buildinName, "read_num") == 0) {
        printf("CALL $read_num\n"); // this can stay as is, because it returns to stack anyway
        return;
    }

    // Ifj.floor
    if(strcmp(buildinName, "floor") == 0) {
        codegenPrintExpr(cgCall->args[0], indent + 1, scope, false);
        printf("CALL $floor\n");
        return;
    }

    // Ifj.str
    if (strcmp(buildinName, "str") == 0) {
        codegenPrintExpr(cgCall->args[0], indent + 1, scope, false);
        printf("CALL $str\n");
        return;
    }

    // Ifj.length
    if (strcmp(buildinName, "length") == 0) {
        codegenPrintExpr(cgCall->args[0], indent + 1, scope, false);
        printf("CALL $len\n");
        return;
    }

    // Ifj.readString
    if (strcmp(buildinName, "read_str") == 0) {
        printf("CALL $read_str\n");
        return;
    }

    // Ifj.strcmp
    if (strcmp(buildinName, "strcmp") == 0) {
        codegenPrintExpr(cgCall->args[0], indent + 1, scope, false);
        codegenPrintExpr(cgCall->args[1], indent + 1, scope, false);
        printf("CALL $strcmp\n");
        return;
    }

    // Ifj.ord
    if (strcmp(buildinName, "ord") == 0) {
        codegenPrintExpr(cgCall->args[0], indent + 1, scope, false);
        codegenPrintExpr(cgCall->args[1], indent + 1, scope, false);
        printf("CALL $ord\n");
        return;
    }

    // Ifj.chr
    if (strcmp(buildinName, "chr") == 0) {
        codegenPrintExpr(cgCall->args[0], indent + 1, scope, false);
        printf("CALL $chr\n");
        return;
    }

    // Ifj.substring
    if (strcmp(buildinName, "substring") == 0) {
        codegenPrintExpr(cgCall->args[0], indent + 1, scope, false);
        codegenPrintExpr(cgCall->args[1], indent + 1, scope, false);
        codegenPrintExpr(cgCall->args[2], indent + 1, scope, false);
        printf("CALL $substring\n");
        return;
    }

    if (cgCall->args && cgCall->argsCount > 0) {
        for (int i = 0; i < cgCall->argsCount; i++) {
            if (!cgCall->args[i]) {
                continue;
            }
            codegenPrintExpr(cgCall->args[i], indent + 1, scope, false);
        }
    }   

}

// --- EXPRESSIONS ---
void codegenPrintExpr(Expr* expr, int indent, semtree_node_t *scope, bool* isFloatOperation) {
    if (isFloatOperation == NULL) {
        isFloatOperation = malloc(sizeof(bool));
        if (isFloatOperation == NULL) {
            fprintf(stderr, "Internal error: Memory allocation failed.\n");
            exit(INTERNAL_ERROR);
        }
        *isFloatOperation = false;
    }
    if (!expr) return;
    switch (expr->type) {
        case funcCallExpr:
            codegenPrintFuncCall(expr->expr, indent + 1, scope);                // Function call
            break;
        case builtinCallExpr:
            codegenPrintBuiltinCall(expr->expr, indent + 1, scope);             // Builtin call
            break;
        case idExpr: {                                                          // Identifier                           
            IdExpr* id = (IdExpr*)expr->expr;
            if (isGetter(id->id, getGlobalScope(scope)->getters)) {
                printf("JUMP $GETTER_VAR_%s\n", id->id->str);    //TODO:
            } else {
                const char *label = codegenLookupLabel(scope, id->id);
                printf("PUSHS %s\n", label);
            }
            break;
        }
        case numExpr: {                                                         // Number literal                                       
            NumExpr* num = (NumExpr*)expr->expr;
            if (*isFloatOperation || isFloat(num->id)) {
                *isFloatOperation = true;
                double value = atof(num->id->str);
                printf("PUSHS float@%a\n", value);
            }
            else printf("PUSHS int@%s\n", num->id->str);
            break;
        }
        case strExpr: {                                                         // String literal
            StrExpr* str = (StrExpr*)expr->expr;
            printf("PUSHS string@");
            codegenEmitEscapedStringLiteral(str->id);
            printf("\n");
            break;
        }
        case trueExpr:                                                          // Boolean true
            printf("PUSHS bool@true\n");
            break;
        case falseExpr:                                                         // Boolean false
            printf("PUSHS bool@false\n");
            break;
        case nullExpr:                                                          // Null literal 
            printf("PUSHS nil@nil\n");
            break;
        case innerExpr: {                                                       // Inner expression                   
            InnerExpr* inner = (InnerExpr*)expr->expr;
            codegenPrintExpr(inner->expr, indent + 1, scope, isFloatOperation);
            break;
        }
        case exprOpExpr: {                                                      // exp Operator exp
            ExprOpExpr* op = (ExprOpExpr*)expr->expr;
            switch (op->type) {
                case addExpr: codegenExprPlusExpr(op, scope, isFloatOperation); break;              // +
                case subExpr: codegenExprMinusExpr(op, scope, isFloatOperation); break;             // -
                case mulExpr: codegenExprMulExpr(op, scope, isFloatOperation); break;               // *
                case divExpr: codegenExprDivExpr(op, scope, isFloatOperation); break;               // /
                case gtExpr: codegenCompGtExpr(op, scope); break;                                   // >
                case ltExpr: codegenCompLtExpr(op, scope); break;                                   // <
                case geExpr: codegenCompGteExpr(op, scope); break;                                  // >=    
                case leExpr: codegenCompLteExpr(op, scope); break;                                  // <=
                case eqExpr: codegenCompEqExpr(op, scope); break;                                   // ==
                case neqExpr: codegenCompNeqExpr(op, scope); break;                                 // != 
            }
            break;
        }
        case isExpr: {                                                                             // is expression
            IsExpr* is = (IsExpr*)expr->expr;
            codegenPrintExpr(is->testedExpr, indent + 1, scope, false);
            switch (is->type) {
                case isString: {                                                                    // is String
                    printf("TYPES\n");
                    printf("PUSHS string@string\n");
                    printf("EQS\n");
                    break;
                } 
                case isNum: {                                                                       // is Num
                    printf("TYPES\n");
                    printf("PUSHS string@int\n");
                    printf("EQS\n");
                    break;
                }
                case isNull: {                                                                       // is Null
                    printf("TYPES\n");
                    printf("PUSHS string@nil\n");
                    printf("EQS\n");
                    break;
                }
            }
            break;
        }
    }
}

void codegenIsExprStringExpr(IsExpr* isExpr) {
    
}


void codegenExprPlusExpr(ExprOpExpr* exprOpExpr, semtree_node_t *scope, bool* isFloatOperation) {   // +
    if (isFloatOperation == NULL) {
        isFloatOperation = malloc(sizeof(bool));
        if (isFloatOperation == NULL) {
            fprintf(stderr, "Internal error: Memory allocation failed.\n");
            exit(INTERNAL_ERROR);
        }
        *isFloatOperation = false;
    }

    // Check if any operand is float
    if (exprOpExpr->expr1->type == numExpr) {                                                       
        NumExpr* expr = exprOpExpr->expr1->expr;
        if (isFloat(expr->id)) *isFloatOperation = true;
    }
    if (exprOpExpr->expr2->type == numExpr) {
        NumExpr* expr = exprOpExpr->expr2->expr;
        if (isFloat(expr->id)) *isFloatOperation = true;
    }  

    bool leftDefStr = codegenExprDefString(exprOpExpr->expr1);                      
    bool rightDefStr = codegenExprDefString(exprOpExpr->expr2);

    // If both sides are definitely strings, do concat directly
    if (leftDefStr && rightDefStr) {
        printf("PUSHFRAME\n");
        printf("CREATEFRAME\n");
        printf("DEFVAR TF@expr1\n");
        codegenPrintExpr(exprOpExpr->expr1, 0, scope, isFloatOperation);
        printf("POPS TF@expr1\n");
        printf("DEFVAR TF@expr2\n");
        codegenPrintExpr(exprOpExpr->expr2, 0, scope, isFloatOperation);
        printf("POPS TF@expr2\n");
        printf("DEFVAR TF@result\n");
        printf("CONCAT TF@result TF@expr1 TF@expr2\n");
        printf("PUSHS TF@result\n");
        printf("POPFRAME\n");
        return;
    }

    codegenPrintExpr(exprOpExpr->expr1, 0, scope, isFloatOperation);
    codegenPrintExpr(exprOpExpr->expr2, 0, scope, isFloatOperation);

    addCount++;
    int currentAddCount = addCount;

    printf("PUSHFRAME\n");
    printf("CREATEFRAME\n");
    printf("DEFVAR TF@expr1\n");
    printf("DEFVAR TF@expr2\n");
    printf("POPS TF@expr2\n");
    printf("POPS TF@expr1\n");
    printf("DEFVAR TF@type1\n");
    printf("DEFVAR TF@type2\n");
    printf("TYPE TF@type1 TF@expr1\n");
    printf("TYPE TF@type2 TF@expr2\n");
    printf("JUMPIFNEQ $$ADD_NUM_%d TF@type1 string@string\n", currentAddCount);
    printf("JUMPIFNEQ $$ADD_NUM_%d TF@type2 string@string\n", currentAddCount);
    printf("JUMP $$ADD_CONCAT_%d\n", currentAddCount);
    printf("LABEL $$ADD_NUM_%d\n", currentAddCount);
    printf("PUSHS TF@expr1\n");
    printf("PUSHS TF@expr2\n");
    printf("POPFRAME\n");
    printf("ADDS\n");
    printf("JUMP $$ADD_END_%d\n", currentAddCount);
    printf("LABEL $$ADD_CONCAT_%d\n", currentAddCount);
    printf("DEFVAR TF@result\n");
    printf("CONCAT TF@result TF@expr1 TF@expr2\n");
    printf("PUSHS TF@result\n");
    printf("POPFRAME\n");
    printf("LABEL $$ADD_END_%d\n", currentAddCount);
}

void codegenExprMinusExpr(ExprOpExpr* exprOpExpr, semtree_node_t *scope, bool* isFloatOperation) {  // -
    if (isFloatOperation == NULL) {
        isFloatOperation = malloc(sizeof(bool));
        if (isFloatOperation == NULL) {
            fprintf(stderr, "Internal error: Memory allocation failed.\n");
            exit(INTERNAL_ERROR);
        }
        *isFloatOperation = false;
    }
    if (exprOpExpr->expr1->type == numExpr) {
        NumExpr* expr = exprOpExpr->expr1->expr;
        if (isFloat(expr->id)) *isFloatOperation = true;
    }
    if (exprOpExpr->expr2->type == numExpr) {
        NumExpr* expr = exprOpExpr->expr2->expr;
        if (isFloat(expr->id)) *isFloatOperation = true;
    }  
    
    codegenPrintExpr(exprOpExpr->expr1, 0, scope, isFloatOperation);
    codegenPrintExpr(exprOpExpr->expr2, 0, scope, isFloatOperation);
    printf("SUBS\n");
}

void codegenExprDivExpr(ExprOpExpr* exprOpExpr, semtree_node_t *scope, bool* isFloatOperation) {   // /
    if (isFloatOperation == NULL) {
        isFloatOperation = malloc(sizeof(bool));
        if (isFloatOperation == NULL) {
            fprintf(stderr, "Internal error: Memory allocation failed.\n");
            exit(INTERNAL_ERROR);
        }
        *isFloatOperation = false;
    }
    if (exprOpExpr->expr1->type == numExpr) {
        NumExpr* expr = exprOpExpr->expr1->expr;
        if (isFloat(expr->id)) *isFloatOperation = true;
    }
    if (exprOpExpr->expr2->type == numExpr) {
        NumExpr* expr = exprOpExpr->expr2->expr;
        if (isFloat(expr->id)) *isFloatOperation = true;
    }  

    codegenPrintExpr(exprOpExpr->expr1, 0, scope, isFloatOperation);
    codegenPrintExpr(exprOpExpr->expr2, 0, scope, isFloatOperation);
    if (*isFloatOperation) printf("DIVS\n");
    else printf("IDIVS\n");
}

void codegenExprMulExpr(ExprOpExpr* exprOpExpr, semtree_node_t *scope, bool* isFloatOperation) {
    if (isFloatOperation == NULL) {
        isFloatOperation = malloc(sizeof(bool));
        if (isFloatOperation == NULL) {
            fprintf(stderr, "Internal error: Memory allocation failed.\n");
            exit(INTERNAL_ERROR);
        }
        *isFloatOperation = false;
    }
    if (exprOpExpr->expr1->type == numExpr) {
        NumExpr* expr = exprOpExpr->expr1->expr;
        if (isFloat(expr->id)) *isFloatOperation = true;
    }
    if (exprOpExpr->expr2->type == numExpr) {
        NumExpr* expr = exprOpExpr->expr2->expr;
        if (isFloat(expr->id)) *isFloatOperation = true;
    }  

    // Again, checks if the first operand is definitely a string
    codegenPrintExpr(exprOpExpr->expr1, 0, scope, isFloatOperation);
    codegenPrintExpr(exprOpExpr->expr2, 0, scope, isFloatOperation);


    mulCount++;
    int currentMulCount = mulCount;

    printf("PUSHFRAME\n");
    printf("CREATEFRAME\n");
    printf("DEFVAR TF@expr1\n");
    printf("DEFVAR TF@expr2\n");
    printf("POPS TF@expr2\n");
    printf("POPS TF@expr1\n");
    printf("DEFVAR TF@type1\n");
    printf("TYPE TF@type1 TF@expr1\n");
    printf("JUMPIFEQ $$MUL_STRING_%d TF@type1 string@string\n", currentMulCount);
    
    printf("PUSHS TF@expr1\n");
    printf("PUSHS TF@expr2\n");
    printf("POPFRAME\n");
    printf("MULS\n");
    printf("JUMP $$MUL_END_%d\n", currentMulCount);
    
    printf("LABEL $$MUL_STRING_%d\n", currentMulCount);
    printf("DEFVAR TF@result\n");
    printf("MOVE TF@result string@\n");
    printf("DEFVAR TF@counter\n");
    printf("MOVE TF@counter TF@expr2\n");
    
    printf("LABEL $$MUL_LOOP_%d\n", currentMulCount);
    printf("GT TF@type1 TF@counter int@0\n");
    printf("JUMPIFEQ $$MUL_LOOP_BODY_%d TF@type1 bool@true\n", currentMulCount);
    printf("JUMP $$MUL_LOOP_END_%d\n", currentMulCount);
    
    printf("LABEL $$MUL_LOOP_BODY_%d\n", currentMulCount);
    printf("CONCAT TF@result TF@result TF@expr1\n");
    printf("SUB TF@counter TF@counter int@1\n");
    printf("JUMP $$MUL_LOOP_%d\n", currentMulCount);
    
    printf("LABEL $$MUL_LOOP_END_%d\n", currentMulCount);
    printf("PUSHS TF@result\n");
    printf("POPFRAME\n");
    
    printf("LABEL $$MUL_END_%d\n", currentMulCount);
}

// --- COMPARISON EXPRESSIONS ---
void codegenCompGtExpr(ExprOpExpr* exprOpExpr, semtree_node_t *scope) {                 // >
    codegenPrintExpr(exprOpExpr->expr1, 0, scope, false);
    codegenPrintExpr(exprOpExpr->expr2, 0, scope, false);
    printf("GTS\n");
}

void codegenCompLtExpr(ExprOpExpr* exprOpExpr, semtree_node_t *scope) {                 // <
    codegenPrintExpr(exprOpExpr->expr1, 0, scope, false);
    codegenPrintExpr(exprOpExpr->expr2, 0, scope, false);
    printf("LTS\n");
}

void codegenCompEqExpr(ExprOpExpr* exprOpExpr, semtree_node_t *scope) {                 // ==
    codegenPrintExpr(exprOpExpr->expr1, 0, scope, false);
    codegenPrintExpr(exprOpExpr->expr2, 0, scope, false);
    printf("EQS\n");
}

void codegenCompLteExpr(ExprOpExpr* exprOpExpr, semtree_node_t *scope) {                 // <=  
    codegenPrintExpr(exprOpExpr->expr1, 0, scope, false);
    codegenPrintExpr(exprOpExpr->expr2, 0, scope, false);
    printf("GTS\n");
    printf("NOTS\n");
}

void codegenCompGteExpr(ExprOpExpr* exprOpExpr, semtree_node_t *scope) {                // >=
    codegenPrintExpr(exprOpExpr->expr1, 0, scope, false);
    codegenPrintExpr(exprOpExpr->expr2, 0, scope, false);
    printf("LTS\n");
    printf("NOTS\n");
}

void codegenCompNeqExpr(ExprOpExpr* exprOpExpr, semtree_node_t *scope) {                // !=
    codegenPrintExpr(exprOpExpr->expr1, 0, scope, false);
    codegenPrintExpr(exprOpExpr->expr2, 0, scope, false);
    printf("EQS\n");
    printf("NOTS\n");
}


// --- BODY ---
void codegenPrintBody(Body* ast, int indent, semtree_node_t *scope) {
    for (Body* cgCmd = ast; cgCmd != NULL; cgCmd = cgCmd->nextCmd) {

        switch (cgCmd->type) {
            //  --- IF STATEMENT ---
            case ifStm:
            {
                IfStm *ifCmd = (IfStm*)cgCmd->cmd;
                semtree_node_t *ifScope = ifCmd && ifCmd->ifBody ? codegenAcquireChildScope(scope, "if") : NULL;
                semtree_node_t *elseScope = ifCmd && ifCmd->elseBody ? codegenAcquireChildScope(scope, "else") : NULL;
                codegenPringIfStm(cgCmd->cmd, indent + 1, scope, ifScope, elseScope);
            break;
            }
                
            //  --- RETURN ---
            case ret:
                codegenPrintRet(cgCmd->cmd, indent + 1, scope); // This can be here - result is returned to stack
                printf("POPFRAME\n");
                printf("RETURN\n");
            break;

            //  --- VARIABLE DECLARATION ---
            case varDecl:
                codegenPrintVarDecl(cgCmd->cmd, indent + 1, scope);
            break;
            

            //  --- WHILE LOOP ---
            case whileLoop:
            {
                While *whileCmd = (While*)cgCmd->cmd;
                semtree_node_t *loopScope = whileCmd && whileCmd->body ? codegenAcquireChildScope(scope, "while") : NULL;
                codegenPrintWhileLoop(cgCmd->cmd, indent + 1, scope, loopScope);
                break;
            }
            
            // --- ASSIGNMENT ---
            case assign:
                codegenPrintAssign(cgCmd->cmd, indent + 1, scope);
                break;
            

            // --- BLOCK ---
            case block:
            {
                if (cgCmd->cmd) {
                    semtree_node_t *blockScope = codegenAcquireChildScope(scope, "block");
                    codegenPrintBlock(cgCmd->cmd, indent + 1, blockScope);
                }
            }
            break;
            
            // --- FUNCTION CALL ---
            case funcCall:
                codegenPrintFuncCall(cgCmd->cmd, indent + 1, scope);
            break;
            
            // --- BUILTIN CALL ---
            case builtinCall:
                codegenPrintBuiltinCall(cgCmd->cmd, indent + 1, scope);
            break;
        }
    }
}

// --- FUNCTION DECLARATION ---
void codegenPrintFuncDecl(FuncDecl* ast, semtree_node_t *scope) {
    printf("LABEL $%s$$%i\n", ast->id->str, ast->paramsCount);
    printf("CREATEFRAME\n");
    printf("PUSHFRAME\n");
    printf("CREATEFRAME\n");

    codegenDeclareLocalVars(scope);

    for (int i = 0; i < ast->paramsCount; i++) {
        const char *label = codegenLookupLabel(scope, ast->params[i]);
        printf("POPS %s\n", label);
    }

    codegenPrintBody(ast->body, 1, scope);

    printf("POPFRAME\n");

    printf("PUSHS int@0\n");
    printf("RETURN\n");
}


// --- GETTER / SETTER ---
void codegenPrintGetter(Getter* ast, semtree_node_t *scope) {
    printf("LABEL $GETTER_VAR_%s\n", ast->id->str);
    printf("PUSHFRAME\n");
    printf("CREATEFRAME\n");
    
    codegenPrintBody(ast->body, 1, scope);

    printf("POPFRAME\n");
    printf("RETURN\n");
}

void codegenPrintSetter(Setter* ast, semtree_node_t *scope) {
    printf("LABEL $SETTER_VAR_%s\n", ast->id->str);
    printf("PUSHFRAME\n");
    printf("CREATEFRAME\n");
    printf("DEFVAR TF@param\n");
    printf("POPS TF@param\n");

    if (ast->param && scope) {
        const char *label = codegenLookupLabel(scope, ast->param);
        printf("MOVE %s TF@param\n", label);
    }

    codegenPrintBody(ast->body, 1, scope);

    printf("POPFRAME\n");
    printf("RETURN\n");
}

// Main entery point
void codegenPrint(Code* ast, semtree_node_t* globalScope) {

    // Checks
    if (!globalScope) {
        fprintf(stderr, "Codegen error: missing global scope information.\n");
        return;
    }

    int dbg = semtreePrepareCodegenLabels(globalScope);
    if (dbg != 0) {
        fprintf(stderr, "Codegen error: failed to prepare scope labels, err %d\n", dbg);
        return;
    }


    semtreeResetCodegenCursors(globalScope);        
    pasteStartInstructionFirst();                     // 1/2 Preamble
    codegenDeclarGlobalVars(globalScope);             // Global variable declarations
    pasteStartInstructionSec();                       // 2/2 Preamble
    generateBuiltinFunctions();                       // Builtin functions        


    for (Code* globCmd = ast; globCmd != NULL; globCmd = globCmd->nextGlobalCmd) {
        switch (globCmd->type) {
            case funcDecl:
            {
                FuncDecl *fn = globCmd->globCmd;
                semtree_node_t *fnScope = semtreeFindChildByName(globalScope, fn ? fn->id : NULL);
                codegenPrintFuncDecl(fn, fnScope);
            }
            break;
                
            case getter:
            {
                Getter *g = globCmd->globCmd;
                semtree_node_t *gScope = semtreeFindChildByName(globalScope, g ? g->id : NULL);
                codegenPrintGetter(g, gScope);
            }
            break;
            
            case setter:
            {
                Setter *s = globCmd->globCmd;
                semtree_node_t *sScope = semtreeFindChildByName(globalScope, s ? s->id : NULL);
                codegenPrintSetter(s, sScope);
            }
            break;
        }
    }

    printf("LABEL $$end\n");
}


// --- VARIABLE DECLARATION HELPERS ---
void codegenEmitDefvar(String *name, const char *label, void *userData) {
    (void)name;
    (void)userData;
    if (label) {
        printf("DEFVAR %s\n", label);
    }
}

// Declare global variables
void codegenDeclarGlobalVars(semtree_node_t* scope) {
    if (!scope) return;
    semtreeForEachSymbol(scope, codegenEmitDefvar, NULL);
}

// Declare local variables recursively in the scope tree
void codegenDeclareLocalVars(semtree_node_t* scope) {
    if (!scope) return;
    semtreeForEachSymbol(scope, codegenEmitDefvar, NULL);
    for (size_t i = 0; i < scope->childCount; i++) {
        codegenDeclareLocalVars(scope->children[i]);
    }
}

// --- STRING LITERAL ESCAPING ---
void codegenEmitEscapedStringLiteral(String* s) {
    if (!s) { return; }
    const char* p = s->str;
    while (*p) {
        unsigned char c = (unsigned char)*p;
        switch (c) {
            case ' ':
                printf("\\032");
                break;
            case '\\':
                printf("\\092"); 
                break;
            case '\n':
                printf("\\010");
                break;
            case '\t':
                printf("\\009");
                break;
            default:
                if (c < 32 || c == 127) {
                    printf("\\%03o", c);
                } else {
                    putchar(c);
                }
                break;
        }
        p++;
    }
}