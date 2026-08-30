/***************************************************
 * Project: Implementation of the IFJ25 language compiler
 * File   : ast-visualiser.c
 * Author : Jaroslav Synek (xsynekj00)
 * Date   : 2. 12. 2025
 *
 * Description: Abstract synax tree visualizer
 ***************************************************/
    

#include <stdio.h>

#include "ast-visualiser.h"

void printExpr(Expr* expr, int indent);
void printBody(Body* ast, int indent);

void printIfStm(void* cmd, int indent) {
    IfStm* ifStm = (IfStm*)cmd;
    for (int i = 0; i < indent; i++) printf("  ");
    printf("cond:\n");
    printExpr(ifStm->cond, indent + 1);
    for (int i = 0; i < indent; i++) printf("  ");
    printf("ifBody:\n");
    printBody(ifStm->ifBody, indent + 1);
    if (ifStm->elseBody) {
        for (int i = 0; i < indent; i++) printf("  ");
        printf("elseBody:\n");
        printBody(ifStm->elseBody, indent + 1);
    }
}

void printRet(void* cmd, int indent) {
    Ret* ret = (Ret*)cmd;
    for (int i = 0; i < indent; i++) printf("  ");
    printf("expr:\n");
    printExpr(ret->expr, indent + 1);
}

void printVarDecl(void* cmd, int indent) {
    VarDecl* varDecl = (VarDecl*)cmd;
    for (int i = 0; i < indent; i++) printf("  ");
    printf("id: %s\n", varDecl->id->str);
    for (int i = 0; i < indent; i++) printf("  ");
    printf("expr:\n");
    printExpr(varDecl->expr, indent + 1);
}

void printWhileLoop(void* cmd, int indent) {
    While* whileLoop = (While*)cmd;
    for (int i = 0; i < indent; i++) printf("  ");
    printf("cond:\n");
    printExpr(whileLoop->expr, indent + 1);
    for (int i = 0; i < indent; i++) printf("  ");
    printf("body:\n");
    printBody(whileLoop->body, indent + 1);
}

void printAssign(void* cmd, int indent) {
    Assign* assign = (Assign*)cmd;
    for (int i = 0; i < indent; i++) printf("  ");
    printf("id: %s\n", assign->id->str);
    for (int i = 0; i < indent; i++) printf("  ");
    printf("expr:\n");
    printExpr(assign->expr, indent + 1);
}

void printBlock(void* cmd, int indent) {
    Body* block = (Body*)cmd;
    printBody(block, indent + 1);
}

void printFuncCall(void* cmd, int indent) {
    FuncCall* call = (FuncCall*)cmd;
    for (int i = 0; i < indent; i++) printf("  ");
    printf("id: %s\n", call->id->str);
    for (int i = 0; i < indent; i++) printf("  ");
    printf("args:\n");
    if (call->args) {
        for (int i = 0; i < call->argsCount; i++) {
            printExpr(call->args[i], indent + 1);
        }
    }
}

void printBuiltinCall(void* cmd, int indent) {
    BuiltinCall* call = (BuiltinCall*)cmd;
    for (int i = 0; i < indent; i++) printf("  ");
    printf("id: %s\n", call->id->str);
    for (int i = 0; i < indent; i++) printf("  ");
    printf("args:\n");
    if (call->args) {
        for (int i = 0; i < call->argsCount; i++) {
            printExpr(call->args[i], indent + 1);
        }
    }
}

void printExpr(Expr* expr, int indent) {
    if (!expr) return;
    for (int i = 0; i < indent; i++) printf("  ");
    switch (expr->type) {
        case funcCallExpr:
            printf("funcCallExpr\n");
            printFuncCall(expr->expr, indent + 1);
            break;
        case builtinCallExpr:
            printf("builtinCallExpr\n");
            printBuiltinCall(expr->expr, indent + 1);
            break;
        case idExpr: {
            IdExpr* id = (IdExpr*)expr->expr;
            printf("idExpr: %s\n", id->id->str);
            break;
        }
        case numExpr: {
            NumExpr* num = (NumExpr*)expr->expr;
            printf("numExpr: %s\n", num->id->str);
            break;
        }
        case strExpr: {
            StrExpr* str = (StrExpr*)expr->expr;
            printf("strExpr: %s\n", str->id->str);
            break;
        }
        case trueExpr:
            printf("trueExpr\n");
            break;
        case falseExpr:
            printf("falseExpr\n");
            break;
        case nullExpr:
            printf("nullExpr\n");
            break;
        case innerExpr: {
            InnerExpr* inner = (InnerExpr*)expr->expr;
            printf("innerExpr\n");
            printExpr(inner->expr, indent + 1);
            break;
        }
        case exprOpExpr: {
            ExprOpExpr* op = (ExprOpExpr*)expr->expr;
            printf("exprOpExpr: ");
            switch (op->type) {
                case addExpr: printf("+\n"); break;
                case subExpr: printf("-\n"); break;
                case mulExpr: printf("*\n"); break;
                case divExpr: printf("/\n"); break;
                case gtExpr: printf(">\n"); break;
                case ltExpr: printf("<\n"); break;
                case geExpr: printf(">=\n"); break;
                case leExpr: printf("<=\n"); break;
                case eqExpr: printf("==\n"); break;
                case neqExpr: printf("!=\n"); break;
            }
            printExpr(op->expr1, indent + 1);
            printExpr(op->expr2, indent + 1);
            break;
        }
        case isExpr: {
            IsExpr* is = (IsExpr*)expr->expr;
            printf("isExpr: ");
            switch (is->type) {
                case isString: printf("string\n"); break;
                case isNum: printf("num\n"); break;
                case isNull: printf("null\n"); break;
            }
            printExpr(is->testedExpr, indent + 1);
            break;
        }
    }
}

void printBody(Body* ast, int indent) {
    for (Body* cmd = ast; cmd != NULL; cmd = cmd->nextCmd) {
        for (int i = 0; i < indent; i++) printf("  ");
        switch (cmd->type) {
            case ifStm:
                printf("ifStm\n");
                printIfStm(cmd->cmd, indent + 1);
            break;
                
            case ret:
                printf("ret\n");
                printRet(cmd->cmd, indent + 1);
            break;

            case varDecl:
                printf("varDecl\n");
                printVarDecl(cmd->cmd, indent + 1);
            break;
            
            case whileLoop:
                printf("whileLoop\n");
                printWhileLoop(cmd->cmd, indent + 1);
            break;
            
            case assign:
                printf("assign\n");
                printAssign(cmd->cmd, indent + 1);
            break;
            
            case block:
                printf("block\n");
                printBlock(cmd->cmd, indent + 1);
            break;
            
            case funcCall:
                printf("funcCall\n");
                printFuncCall(cmd->cmd, indent + 1);
            break;
            
            case builtinCall:
                printf("builtinCall\n");
                printBuiltinCall(cmd->cmd, indent + 1);
            break;
        }
    }
}

void printFuncDecl(FuncDecl* ast) {
    printf("%s\n", ast->id->str);

    printf("  params:\n");

    for (int i = 0; i < ast->paramsCount; i++) {
        printf("    %s\n", ast->params[i]->str);
    }
    printf("{");

    printBody(ast->body, 1);

    printf("}\n");
}

void printGetter(Getter* ast) {
    printf("%s\n", ast->id->str);
    
    printf("{");

    printBody(ast->body, 1);

    printf("}\n");   
}

void printSetter(Setter* ast) {
    printf("%s\n", ast->id->str);

    printf("  param: %s\n", ast->param->str);

    printf("{");

    printBody(ast->body, 1);

    printf("}\n");
}

void printAST(Code* ast) {
    for (Code* globCmd = ast; globCmd != NULL; globCmd = globCmd->nextGlobalCmd) {
        switch (globCmd->type) {
            case funcDecl:
                printf("funcDecl ");
                printFuncDecl(globCmd->globCmd);
            break;
                
            case getter:
                printf("getter ");
                printGetter(globCmd->globCmd);
            break;
            
            case setter:
                printf("setter ");
                printSetter(globCmd->globCmd);
            break;
        }
    }
    
}