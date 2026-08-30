/***************************************************
 * Project: Implementation of the IFJ25 language compiler
 * File   : ast.c
 * Author : Jaroslav Synek (xsynekj00)
 * Date   : 2. 12. 2025
 *
 * Description: Abstract syntax tree implementation
 ***************************************************/

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "ast.h"
#include "lexer.h"
#include "error.h"
#include "debug_utils.h"
#include "stack.h"

const char precedenceTable[18][18] = {
    //                 0    1    2    3    4    5    6    7    8    9    10   11  12   13   14    15   16   17
    //                 *    /    +    -    <    >    <=   >=   is   ==   !=   i   Null Num  Str   (    )    $
    /* *    0   */  { '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '<', ' ', ' ', ' ', '<', '>', '>' },
    /* /    1   */  { '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '<', ' ', ' ', ' ', '<', '>', '>' },
    /* +    2   */  { '<', '<', '>', '>', '>', '>', '>', '>', '>', '>', '>', '<', ' ', ' ', ' ', '<', '>', '>' },
    /* -    3   */  { '<', '<', '>', '>', '>', '>', '>', '>', '>', '>', '>', '<', ' ', ' ', ' ', '<', '>', '>' },
    /* <    4   */  { '<', '<', '<', '<', '>', '>', '>', '>', '>', '>', '>', '<', ' ', ' ', ' ', '<', '>', '>' },
    /* >    5   */  { '<', '<', '<', '<', '>', '>', '>', '>', '>', '>', '>', '<', ' ', ' ', ' ', '<', '>', '>' },
    /* <=   6   */  { '<', '<', '<', '<', '>', '>', '>', '>', '>', '>', '>', '<', ' ', ' ', ' ', '<', '>', '>' },
    /* >=   7   */  { '<', '<', '<', '<', '>', '>', '>', '>', '>', '>', '>', '<', ' ', ' ', ' ', '<', '>', '>' },
    /* is   8   */  { '<', '<', '<', '<', '<', '<', '<', '<', '>', '>', '>', ' ', '=', '=', '=', ' ', '>', '>' },
    /* ==   9   */  { '<', '<', '<', '<', '<', '<', '<', '<', '<', '>', '>', '<', ' ', ' ', ' ', '<', '>', '>' },
    /* !=   10  */  { '<', '<', '<', '<', '<', '<', '<', '<', '<', '>', '>', '<', ' ', ' ', ' ', '<', '>', '>' },
    /* i    11  */  { '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', ' ', ' ', ' ', ' ', 'F', '>', '>' },
    /* Null 12  */  { ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '>', '>', '>', ' ', ' ', ' ', ' ', ' ', '>', '>' },
    /* Num  13  */  { ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '>', '>', '>', ' ', ' ', ' ', ' ', ' ', '>', '>' },
    /* Str  14  */  { ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '>', '>', '>', ' ', ' ', ' ', ' ', ' ', '>', '>' },
    /* (    15  */  { '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', ' ', ' ', ' ', '<', '=', ' ' },
    /* )    16  */  { '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', ' ', ' ', ' ', ' ', ' ', '>', '>' },
    /* $    17  */  { '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', ' ', ' ', ' ', '<', ' ', ' ' }
};

int classify(Token* t) {
    if (t == NULL) return 17;

    // literals and true/false/null
    if (t->type == TOKEN_NUMBER_LITERAL ||
        t->type == TOKEN_STRING_LITERAL ||
        (t->type == TOKEN_KEYWORD &&
         (strcmp(t->param->str, "true") == 0  ||
          strcmp(t->param->str, "false") == 0 ||
          strcmp(t->param->str, "null") == 0)))
        return 11;

    // Null / Num / String
    if (t->type == TOKEN_KEYWORD && strcmp(t->param->str, "Null") == 0) return 12;
    if (t->type == TOKEN_KEYWORD && strcmp(t->param->str, "Num") == 0) return 13;
    if (t->type == TOKEN_KEYWORD && strcmp(t->param->str, "String") == 0) return 14;

    // operators
    token_type terminals[18] = {
        TOKEN_MUL,
        TOKEN_DIV,
        TOKEN_ADD,
        TOKEN_SUB,
        TOKEN_LESS,
        TOKEN_GREATER,
        TOKEN_LESS_EQUAL,
        TOKEN_GREATER_EQUAL,
        TOKEN_KEYWORD,       // is
        TOKEN_EQUAL,
        TOKEN_NOT_EQUAL,
        TOKEN_IDENTIFIER,
        TOKEN_KEYWORD,       // Null type?
        TOKEN_KEYWORD,       // Num type?
        TOKEN_KEYWORD,       // String type?
        TOKEN_LEFT_PAREN,
        TOKEN_RIGHT_PAREN,
        TOKEN_NEWLINE
    };

    for (int i = 0; i < 18; i++)
        if (t->type == terminals[i])
            return i;

    return 17;
}


char getPrecedence(Token* a, Token* b) {
    int a_i = classify(a);
    int b_i = classify(b);
    return precedenceTable[a_i][b_i];
    
}

Token* getTokenAndSkipEmptyLines(FILE* file) {
    Token* token;
    do {
        token = getToken(file);
    } while (token->type != TOKEN_EOF && token->type == TOKEN_NEWLINE);

    return token;
}

void parseExpr(FILE* file, Expr** ast, Token** token);

void parseBuiltinCall(FILE* file, Expr** ast, Token** token) {
    *token = getToken(file);
    if ((*token)->type != TOKEN_DOT) {
        fprintf(stderr, "Syntax error: Expected DOT after Ifj got %s.\n", token2String((*token)->type));
        exit(SYNTAX_ERROR);
    }

    *token = getTokenAndSkipEmptyLines(file);
    if ((*token)->type != TOKEN_IDENTIFIER) {
        fprintf(stderr, "Syntax error: Expected IDENTIFIER after Ifj. got %s.\n", token2String((*token)->type));
        exit(SYNTAX_ERROR);
    }
    String* builtinId = (*token)->param;

    *token = getToken(file);
    if ((*token)->type != TOKEN_LEFT_PAREN) {
        fprintf(stderr, "Syntax error: Expected LEFT_PAREN in builtin call got %s.\n", token2String((*token)->type));
        exit(SYNTAX_ERROR);
    }

    Expr** argsTmp = malloc(sizeof(Expr*) * MAX_PARAMS_COUNT);
    if (argsTmp == NULL) {
        fprintf(stderr, "Internal error: Memory allocation failed.\n");
        exit(INTERNAL_ERROR);
    }
    int argc = 0;

    *token = getToken(file);

    if ((*token)->type != TOKEN_RIGHT_PAREN) {
        while (1) {
            if ((*token)->type == TOKEN_KEYWORD && (*token)->param && strcmp((*token)->param->str, "Ifj") == 0) {
                parseBuiltinCall(file, &argsTmp[argc++], token);
            } else {
                parseExpr(file, &argsTmp[argc++], token);
            }

            if (argc >= MAX_PARAMS_COUNT) {
                fprintf(stderr, "Internal error: Too many builtin call arguments.\n");
                exit(INTERNAL_ERROR);
            }

            if ((*token)->type == TOKEN_COMMA) {
                *token = getTokenAndSkipEmptyLines(file); 
                continue;
            } else if ((*token)->type == TOKEN_RIGHT_PAREN) {
                break;
            } else {
                fprintf(stderr, "Syntax error: Expected COMMA or RIGHT_PAREN in builtin call got %s.\n", token2String((*token)->type));
                exit(SYNTAX_ERROR);
            }
        }
    }


    BuiltinCall* call = malloc(sizeof(BuiltinCall));
    if (call == NULL) {
        fprintf(stderr, "Internal error: Memory allocation failed.\n");
        exit(INTERNAL_ERROR);
    }
    call->id = builtinId;
    call->argsCount = argc;
    if (argc > 0) {
        Expr** finalArgs = malloc(sizeof(Expr*) * argc);
        if (finalArgs == NULL) {
            fprintf(stderr, "Internal error: Memory allocation failed.\n");
            exit(INTERNAL_ERROR);
        }
        for (int i = 0; i < argc; ++i) finalArgs[i] = argsTmp[i];
        call->args = finalArgs;
    } else {
        call->args = NULL;
    }

    Expr* exprNode = malloc(sizeof(Expr));
    if (exprNode == NULL) {
        fprintf(stderr, "Internal error: Memory allocation failed.\n");
        exit(INTERNAL_ERROR);
    }
    exprNode->type = builtinCallExpr;
    exprNode->expr = call;
    *ast = exprNode;

    free(argsTmp); 
}

void parseExpr(FILE* file, Expr** ast, Token** token) {
    Token* b = *token;

    Stack* stack = initStack();
    Token* a = NULL;
    while (a != NULL || b != NULL) {
        a = getTop(stack);
        
        char precedence = getPrecedence(a, b);
        switch (precedence) {
            case 'F':
                if (a == NULL || a->type != TOKEN_IDENTIFIER) {
                    fprintf(stderr, "Syntax error: Invalid function call start.\n");
                    exit(SYNTAX_ERROR);
                }

                int funcIdx = -1;
                for (int i = stack->count - 1; i >= 0; --i) {
                    if (stack->data[i]->token == a) {
                        funcIdx = i;
                        break;
                    }
                }

                if (funcIdx < 0) {
                    fprintf(stderr, "Internal error: Function identifier not found on stack.\n");
                    exit(INTERNAL_ERROR);
                }

                Token* currentToken = getToken(file);
                Expr** argsTmp = malloc(sizeof(Expr*) * MAX_PARAMS_COUNT);
                if (argsTmp == NULL) {
                    fprintf(stderr, "Internal error: Memory allocation failed.\n");
                    exit(INTERNAL_ERROR);
                }
                int argc = 0;

                if (currentToken->type != TOKEN_RIGHT_PAREN) {
                    while (1) {
                        if (argc == MAX_PARAMS_COUNT) {
                            fprintf(stderr, "Internal error: Too many function call arguments.\n");
                            exit(INTERNAL_ERROR);
                        }

                        if (currentToken->type == TOKEN_KEYWORD && currentToken->param && strcmp(currentToken->param->str, "Ifj") == 0) {
                            parseBuiltinCall(file, &argsTmp[argc], &currentToken);
                        } else {
                            parseExpr(file, &argsTmp[argc], &currentToken);
                        }
                        argc++;

                        if (currentToken->type == TOKEN_COMMA) {
                            currentToken = getTokenAndSkipEmptyLines(file);
                            continue;
                        } else if (currentToken->type == TOKEN_RIGHT_PAREN) {
                            break;
                        } else {
                            fprintf(stderr, "Syntax error: Expected COMMA or RIGHT_PAREN in function call got %s.\n", token2String(currentToken->type));
                            exit(SYNTAX_ERROR);
                        }
                    }
                }

                if (currentToken->type != TOKEN_RIGHT_PAREN) {
                    fprintf(stderr, "Syntax error: Expected RIGHT_PAREN after function call arguments.\n");
                    exit(SYNTAX_ERROR);
                }

                Token* afterCall = getToken(file);

                FuncCall* funcCall = malloc(sizeof(FuncCall));
                if (funcCall == NULL) {
                    fprintf(stderr, "Internal error: Memory allocation failed.\n");
                    exit(INTERNAL_ERROR);
                }
                funcCall->id = a->param;
                funcCall->argsCount = argc;
                if (argc > 0) {
                    Expr** finalArgs = malloc(sizeof(Expr*) * argc);
                    if (finalArgs == NULL) {
                        fprintf(stderr, "Internal error: Memory allocation failed.\n");
                        exit(INTERNAL_ERROR);
                    }
                    for (int i = 0; i < argc; ++i) {
                        finalArgs[i] = argsTmp[i];
                    }
                    funcCall->args = finalArgs;
                } else {
                    funcCall->args = NULL;
                }

                Expr* funcCallExprNode = malloc(sizeof(Expr));
                if (funcCallExprNode == NULL) {
                    fprintf(stderr, "Internal error: Memory allocation failed.\n");
                    exit(INTERNAL_ERROR);
                }
                funcCallExprNode->type = funcCallExpr;
                funcCallExprNode->expr = funcCall;

                stack->data[funcIdx]->expr = funcCallExprNode;
                stack->data[funcIdx]->precedence = ' ';
                b = afterCall;

                free(argsTmp);
            break;
            case '=':
                if (a != NULL && a->type == TOKEN_LEFT_PAREN && b != NULL && b->type == TOKEN_RIGHT_PAREN) {
                    int a_i = -1;
                    for (int i = stack->count - 1; i >= 0; --i) {
                        if (stack->data[i]->token == a) { a_i = i; break; }
                    }
                    if (a_i < 0) {
                        fprintf(stderr, "Internal error: '(' not found on stack.\n");
                        exit(INTERNAL_ERROR);
                    }

                    if (a_i + 1 >= stack->count || stack->data[a_i + 1]->expr == NULL) {
                        fprintf(stderr, "Syntax error: Invalid inner expression.\n");
                        exit(SYNTAX_ERROR);
                    }

                    Expr* expr = malloc(sizeof(Expr));
                    if (expr == NULL) {
                        fprintf(stderr, "Internal error: Memory allocation failed.\n");
                        exit(INTERNAL_ERROR);
                    }
                    expr->type = innerExpr;
                    InnerExpr* exprData = malloc(sizeof(InnerExpr));
                    if (exprData == NULL) {
                        fprintf(stderr, "Internal error: Memory allocation failed.\n");
                        exit(INTERNAL_ERROR);
                    }
                    exprData->expr = stack->data[a_i + 1]->expr;
                    expr->expr = exprData;

                    stack->data[a_i]->expr = expr;
                    stack->count = a_i + 1;

                    if (b->type >= TOKEN_DOT && b->type <= TOKEN_NOT_EQUAL) b = getTokenAndSkipEmptyLines(file);
                    else b = getToken(file);
                    break;
                }

                push(stack, b, '\0');
                if (b->type >= TOKEN_DOT && b->type <= TOKEN_NOT_EQUAL) b = getTokenAndSkipEmptyLines(file);
                else b = getToken(file);
            break;


            case '<':
                push(stack, b, '<');
                if (b->type >= TOKEN_DOT && b->type <= TOKEN_NOT_EQUAL) b = getTokenAndSkipEmptyLines(file);
                else b = getToken(file);
            break;

            case '>':
                int a_i, b_i;
                for (int i = 0; i < stack->count; i++) {
                    if (stack->data[i]->token == a) {
                        a_i = i;
                    }

                }
                b_i = stack->count;
                
                // built in call
                // function call
                
                if (a_i + 1 == b_i) {
                    // one token
                    // identifier
                    // number literal
                    // string literal
                    // true
                    // false
                    // null
                    Expr* expr = malloc(sizeof(Expr));
                    if (expr == NULL) {
                        fprintf(stderr, "Internal error: Memory allocation failed.\n");
                        exit(INTERNAL_ERROR);
                    }
                    if (a->type == TOKEN_IDENTIFIER) {
                        expr->type = idExpr;
                        IdExpr* exprData = malloc(sizeof(IdExpr));
                        if (exprData == NULL) {
                            fprintf(stderr, "Internal error: Memory allocation failed.\n");
                            exit(INTERNAL_ERROR);
                        }
                        exprData->id = a->param;

                        expr->expr = exprData;
                    }
                    else if (a->type == TOKEN_NUMBER_LITERAL) {
                        expr->type = numExpr;
                        NumExpr* exprData = malloc(sizeof(NumExpr));
                        if (exprData == NULL) {
                            fprintf(stderr, "Internal error: Memory allocation failed.\n");
                            exit(INTERNAL_ERROR);
                        }
                        exprData->id = a->param;

                        expr->expr = exprData;
                    }
                    else if (a->type == TOKEN_STRING_LITERAL) {
                        expr->type = strExpr;
                        StrExpr* exprData = malloc(sizeof(StrExpr));
                        if (exprData == NULL) {
                            fprintf(stderr, "Internal error: Memory allocation failed.\n");
                            exit(INTERNAL_ERROR);
                        }
                        exprData->id = a->param;

                        expr->expr = exprData;
                    }
                    else if (a->type == TOKEN_KEYWORD && strcmp(a->param->str, "true") == 0) {
                        expr->type = trueExpr;
                        expr->expr = NULL;
                    }
                    else if (a->type == TOKEN_KEYWORD && strcmp(a->param->str, "false") == 0) {
                        expr->type = falseExpr;
                        expr->expr = NULL;
                    }
                    else if (a->type == TOKEN_KEYWORD && strcmp(a->param->str, "null") == 0) {
                        expr->type = nullExpr;
                        expr->expr = NULL;
                    } 

                    stack->data[a_i]->expr = expr;
                    stack->data[a_i]->precedence = ' ';
                } else if (a_i + 2 == b_i) {
                    // three tokens
                    // ()
                    // +
                    // -
                    // *
                    // /
                    // <
                    // >
                    // <=
                    // >=
                    // ==
                    // !=
                    // is String
                    // is Num
                    // is Null
                    if (a->type == TOKEN_LEFT_PAREN) {
                        Expr* expr = malloc(sizeof(Expr));
                        if (expr == NULL) {
                            fprintf(stderr, "Internal error: Memory allocation failed.\n");
                            exit(INTERNAL_ERROR);
                        }
                        if (stack->data[a_i + 1]->expr == NULL || b->type != TOKEN_RIGHT_PAREN) {
                            fprintf(stderr, "Syntax error: Invalid inner expression.\n");
                            exit(SYNTAX_ERROR);
                        }
                        expr->type = innerExpr;
                        InnerExpr* exprData = malloc(sizeof(InnerExpr));
                        if (exprData == NULL) {
                            fprintf(stderr, "Internal error: Memory allocation failed.\n");
                            exit(INTERNAL_ERROR);
                        }
                        expr->expr = exprData;
                        stack->count--;     // TODO: massive memory leak
                        stack->data[a_i]->expr = expr;
                        stack->data[a_i]->precedence = ' ';
                    } else if (a_i >= 1 && stack->data[a_i - 1]->expr != NULL && stack->data[a_i + 1]->expr != NULL) {
                        Expr* expr = malloc(sizeof(Expr));
                        if (expr == NULL) {
                            fprintf(stderr, "Internal error: Memory allocation failed.\n");
                            exit(INTERNAL_ERROR);
                        }
                        expr->type = exprOpExpr;
                        ExprOpExpr* exprData = malloc(sizeof(ExprOpExpr));
                        if (exprData == NULL) {
                            fprintf(stderr, "Internal error: Memory allocation failed.\n");
                            exit(INTERNAL_ERROR);
                        }
                        exprData->expr1 = stack->data[a_i - 1]->expr;
                        exprData->expr2 = stack->data[a_i + 1]->expr;

                        if (stack->data[a_i]->token->type == TOKEN_ADD) {
                            exprData->type = addExpr;
                        } else if (stack->data[a_i]->token->type == TOKEN_SUB) {
                            exprData->type = subExpr;
                        } else if (stack->data[a_i]->token->type == TOKEN_MUL) {
                            exprData->type = mulExpr;
                        } else if (stack->data[a_i]->token->type == TOKEN_DIV) {
                            exprData->type = divExpr;
                        } else if (stack->data[a_i]->token->type == TOKEN_LESS) {
                            exprData->type = ltExpr;
                        } else if (stack->data[a_i]->token->type == TOKEN_GREATER) {
                            exprData->type = gtExpr;
                        } else if (stack->data[a_i]->token->type == TOKEN_LESS_EQUAL) {
                            exprData->type = leExpr;
                        } else if (stack->data[a_i]->token->type == TOKEN_GREATER_EQUAL) {
                            exprData->type = geExpr;
                        } else if (stack->data[a_i]->token->type == TOKEN_EQUAL) {
                            exprData->type = eqExpr;
                        } else if (stack->data[a_i]->token->type == TOKEN_NOT_EQUAL) {
                            exprData->type = neqExpr;
                        } else {
                            fprintf(stderr, "Syntax error: Invalid operator in expression.\n");
                            exit(SYNTAX_ERROR);
                        }
                        expr->expr = exprData;
                        stack->count -= 2;     // TODO: massive memory leak
                        stack->data[a_i - 1]->expr = expr;
                        stack->data[a_i - 1]->precedence = ' ';
                    } else if (a_i >= 1 && stack->data[a_i - 1]->expr != NULL && stack->data[a_i]->token->type == TOKEN_KEYWORD &&
                               strcmp(stack->data[a_i]->token->param->str, "is") == 0) {
                        Expr* expr = malloc(sizeof(Expr));
                        if (expr == NULL) {
                            fprintf(stderr, "Internal error: Memory allocation failed.\n");
                            exit(INTERNAL_ERROR);
                        }
                        expr->type = isExpr;
                        IsExpr* exprData = malloc(sizeof(IsExpr));
                        if (exprData == NULL) {
                            fprintf(stderr, "Internal error: Memory allocation failed.\n");
                            exit(INTERNAL_ERROR);
                        }
                        exprData->testedExpr = stack->data[a_i - 1]->expr;
                        
                        if (stack->data[a_i + 1]->token->type == TOKEN_KEYWORD) {
                            if (strcmp(stack->data[a_i + 1]->token->param->str, "String") == 0) {
                                exprData->type = isString;
                            } else if (strcmp(stack->data[a_i + 1]->token->param->str, "Num") == 0) {
                                exprData->type = isNum;
                            } else if (strcmp(stack->data[a_i + 1]->token->param->str, "Null") == 0) {
                                exprData->type = isNull;
                            } else {
                                fprintf(stderr, "Syntax error: Invalid class in expression.\n");
                                exit(SYNTAX_ERROR);
                            }
                            expr->expr = exprData;
                            stack->count -= 2;     // TODO: massive memory leak
                            stack->data[a_i - 1]->precedence = ' ';
                            stack->data[a_i - 1]->expr = expr;
                        } else {
                            fprintf(stderr, "Syntax error: Invalid token in is expression.\n");
                            exit(SYNTAX_ERROR);
                        }
                    } else {
                        fprintf(stderr, "Syntax error: Invalid rule on top of the stack.\n");
                        exit(SYNTAX_ERROR);
                    }
                } else {
                    fprintf(stderr, "Syntax error: Invalid expression.\n");
                    exit(SYNTAX_ERROR);
                }
            break;
            default:
                *token = b;

                Expr* finalExpr = NULL;

                // Find the remaining expression on the stack
                if (stack->count != 1) {
                    fprintf(stderr, "Syntax error: Expression did not reduce to any AST node.\n");
                    exit(SYNTAX_ERROR);
                }

                if (stack->data[stack->count - 1]->expr != NULL) {
                    finalExpr = stack->data[stack->count - 1]->expr;
                }

                if (finalExpr == NULL) {
                    fprintf(stderr, "Syntax error: Expression did not reduce to a single AST node.\n");
                    exit(SYNTAX_ERROR);
                }

                *ast = finalExpr;
                return;
                //printf("-%c- %s %s\n", precedence, token2String(a->type), token2String(b->type));
                //fprintf(stderr, "Syntax error: Invalid expression - invalid end.\n");
                //exit(SYNTAX_ERROR);
        }


    }
}

void parseBody(FILE* file, Body** ast, Token** token) {
    *ast = NULL;
    Body** last = ast;
    *token = getTokenAndSkipEmptyLines(file);

    while ((*token)->type != TOKEN_RIGHT_BRACE) {
        Body* cmd = malloc(sizeof(Body));
        if (cmd == NULL) {
            fprintf(stderr, "Internal error: Memory allocation failed.\n");
            exit(INTERNAL_ERROR);
        }
        void* cmdItself = NULL;
        cmd->nextCmd = NULL;
        cmd->cmd = NULL;

        if (*last != NULL) {
            (*last)->nextCmd = cmd;
        } else {
            *last = cmd;
        }
        last = &cmd->nextCmd;

        if ((*token)->type == TOKEN_KEYWORD) {
            // if statement
            if (strcmp((*token)->param->str, "if") == 0) {
                cmd->type = ifStm;
                cmdItself = malloc(sizeof(IfStm));
                if (cmdItself == NULL) {
                    fprintf(stderr, "Internal error: Memory allocation failed.\n");
                    exit(INTERNAL_ERROR);
                }
                ((IfStm*)cmdItself)->ifBody = NULL;
                ((IfStm*)cmdItself)->elseBody = NULL;
                ((IfStm*)cmdItself)->cond = NULL;
                cmd->cmd = cmdItself;

                *token = getToken(file);
                if ((*token)->type != TOKEN_LEFT_PAREN) {
                    fprintf(stderr, "Syntax error: Expected LEFT_PAREN got %s.\n", token2String((*token)->type));
                    exit(SYNTAX_ERROR);
                }
                
                *token = getToken(file);
                if ((*token)->type == TOKEN_KEYWORD && (*token)->param && strcmp((*token)->param->str, "Ifj") == 0) {
                    parseBuiltinCall(file, &((IfStm *)cmdItself)->cond, token);
                } else {
                    parseExpr(file, &((IfStm *)cmdItself)->cond, token);
                }

                if ((*token)->type != TOKEN_RIGHT_PAREN) {
                    fprintf(stderr, "Syntax error: Expected RIGHT_PAREN got %s.\n", token2String((*token)->type));
                    exit(SYNTAX_ERROR);
                }
                
                *token = getTokenAndSkipEmptyLines(file);
                
                if ((*token)->type != TOKEN_LEFT_BRACE) {
                    fprintf(stderr, "Syntax error: Expected LEFT_BRACE got %s.\n", token2String((*token)->type));
                    exit(SYNTAX_ERROR);
                }
                
                parseBody(file, &((IfStm *)cmdItself)->ifBody, token);
                
                if ((*token)->type != TOKEN_RIGHT_BRACE) {
                    fprintf(stderr, "Syntax error: Expected RIGHT_BRACE got %s.\n", token2String((*token)->type));
                    exit(SYNTAX_ERROR);
                }
                
                *token = getTokenAndSkipEmptyLines(file);
                
                // optional else statement
                if (((*token)->type == TOKEN_KEYWORD) && strcmp((*token)->param->str, "else") == 0) {
                    *token = getTokenAndSkipEmptyLines(file);
                    
                    if ((*token)->type != TOKEN_LEFT_BRACE) {
                        fprintf(stderr, "Syntax error: Expected LEFT_BRACE got %s.\n", token2String((*token)->type));
                        exit(SYNTAX_ERROR);
                    }
                    
                    parseBody(file, &((IfStm *)cmdItself)->elseBody, token);
                    
                    if ((*token)->type != TOKEN_RIGHT_BRACE) {
                        fprintf(stderr, "Syntax error: Expected RIGHT_BRACE got %s.\n", token2String((*token)->type));
                        exit(SYNTAX_ERROR);
                    }
                    
                    *token = getTokenAndSkipEmptyLines(file);
                }
            }
            // return statement
            else if (strcmp((*token)->param->str, "return") == 0) {
                cmd->type = ret;
                cmdItself = malloc(sizeof(Ret));
                if (cmdItself == NULL) {
                    fprintf(stderr, "Internal error: Memory allocation failed.\n");
                    exit(INTERNAL_ERROR);
                }
                ((Ret*)cmdItself)->expr = NULL;
                cmd->cmd = cmdItself;

                *token = getToken(file);
                if ((*token)->type == TOKEN_KEYWORD && (*token)->param && strcmp((*token)->param->str, "Ifj") == 0) {
                    parseBuiltinCall(file, &((Ret*)cmdItself)->expr, token);
                } else if ((*token)->type != TOKEN_NEWLINE) {
                    parseExpr(file, &((Ret*)cmdItself)->expr, token);
                }
                *token = getTokenAndSkipEmptyLines(file);
            }
            // variable declaration
            else if (strcmp((*token)->param->str, "var") ==0) {
                cmd->type = varDecl;
                cmdItself = malloc(sizeof(VarDecl));
                if (cmdItself == NULL) {
                    fprintf(stderr, "Internal error: Memory allocation failed.\n");
                    exit(INTERNAL_ERROR);
                }
                ((VarDecl*)cmdItself)->expr = NULL;
                cmd->cmd = cmdItself;
                *token = getToken(file);
                if ((*token)->type != TOKEN_IDENTIFIER) {
                    fprintf(stderr, "Syntax error: Expected IDENTIFIER got %s.\n", token2String((*token)->type));
                    exit(SYNTAX_ERROR);
                }
                ((VarDecl*)cmdItself)->id = (*token)->param;
                
                *token = getToken(file);
                if ((*token)->type == TOKEN_NEWLINE) {
                    *token = getTokenAndSkipEmptyLines(file);
                    continue;
                }
                else if ((*token)->type != TOKEN_ASSIGN) {
                    fprintf(stderr, "Syntax error: Expected ASSIGN got %s.\n", token2String((*token)->type));
                    exit(SYNTAX_ERROR);
                }
                
                *token = getToken(file);
                if ((*token)->type == TOKEN_KEYWORD && (*token)->param && strcmp((*token)->param->str, "Ifj") == 0) {
                    parseBuiltinCall(file, &((VarDecl*)cmdItself)->expr, token);
                    
                    *token = getToken(file);

                    if ((*token)->type != TOKEN_NEWLINE) {
                        fprintf(stderr, "Syntax error: Expected NEWLINE got %s.\n", token2String((*token)->type));
                        exit(SYNTAX_ERROR);
                    }
                } else {
                    parseExpr(file, &((VarDecl*)cmdItself)->expr, token);
                }

                *token = getTokenAndSkipEmptyLines(file);
            }
            // while loop
            else if (strcmp((*token)->param->str, "while") == 0) {
                cmd->type = whileLoop;
                cmdItself = malloc(sizeof(While));
                if (cmdItself == NULL) {
                    fprintf(stderr, "Internal error: Memory allocation failed.\n");
                    exit(INTERNAL_ERROR);
                }
                ((While*)cmdItself)->body = NULL;
                ((While*)cmdItself)->expr = NULL;
                cmd->cmd = cmdItself;

                *token = getToken(file);
                if ((*token)->type != TOKEN_LEFT_PAREN) {
                    fprintf(stderr, "Syntax error: Expected LEFT_PAREN got %s.\n", token2String((*token)->type));
                    exit(SYNTAX_ERROR);
                }

                *token = getToken(file);

                if ((*token)->type == TOKEN_KEYWORD && (*token)->param && strcmp((*token)->param->str, "Ifj") == 0) {
                    parseBuiltinCall(file, &((While*)cmdItself)->expr, token);
                } else {
                    parseExpr(file, &((While*)cmdItself)->expr, token);
                }

                if ((*token)->type != TOKEN_RIGHT_PAREN) {
                    fprintf(stderr, "Syntax error: Expected RIGHT_PAREN got %s.\n", token2String((*token)->type));
                    exit(SYNTAX_ERROR);
                }

                *token = getTokenAndSkipEmptyLines(file);

                if ((*token)->type != TOKEN_LEFT_BRACE) {
                    fprintf(stderr, "Syntax error: Expected LEFT_BRACE got %s.\n", token2String((*token)->type));
                    exit(SYNTAX_ERROR);
                }

                parseBody(file, &((While*)cmdItself)->body, token);

                if ((*token)->type != TOKEN_RIGHT_BRACE) {
                    fprintf(stderr, "Syntax error: Expected RIGHT_BRACE got %s.\n", token2String((*token)->type));
                    exit(SYNTAX_ERROR);
                }

                *token = getToken(file);

                if ((*token)->type != TOKEN_NEWLINE) {
                    fprintf(stderr, "Syntax error: Expected NEWLINE got %s.\n", token2String((*token)->type));
                    exit(SYNTAX_ERROR);
                }

                *token = getTokenAndSkipEmptyLines(file);
            }
            // built-in function call
            else if (strcmp((*token)->param->str, "Ifj") == 0) {
                cmd->type = builtinCall;
                cmdItself = malloc(sizeof(BuiltinCall));
                if (cmdItself == NULL) {
                    fprintf(stderr, "Internal error: Memory allocation failed.\n");
                    exit(INTERNAL_ERROR);
                }
                ((BuiltinCall*)cmdItself)->args = NULL;
                ((BuiltinCall*)cmdItself)->id = NULL;
                ((BuiltinCall*)cmdItself)->argsCount = 0;
                cmd->cmd = cmdItself;
                Expr** args = malloc (sizeof(Expr) * 3);
                if (args == NULL) {
                    fprintf(stderr, "Internal error: Memory allocation failed.\n");
                    exit(INTERNAL_ERROR);
                }
                int i = 0;
                
                *token = getToken(file);
                if ((*token)->type != TOKEN_DOT) {
                    fprintf(stderr, "Syntax error: Expected DOT got %s.\n", token2String((*token)->type));
                    exit(SYNTAX_ERROR);
                }
                
                *token = getTokenAndSkipEmptyLines(file);
                if ((*token)->type != TOKEN_IDENTIFIER) {
                    fprintf(stderr, "Syntax error: Expected IDENTIFIER got %s.\n", token2String((*token)->type));
                    exit(SYNTAX_ERROR);
                }
                ((BuiltinCall*)cmdItself)->id = (*token)->param;
                
                *token = getToken(file);
                if ((*token)->type != TOKEN_LEFT_PAREN) {
                    fprintf(stderr, "Syntax error: Expected LEFT_PAREN got %s.\n", token2String((*token)->type));
                    exit(SYNTAX_ERROR);
                }

                do {                    
                    if ((*token)->type != TOKEN_COMMA) {
                        *token = getTokenAndSkipEmptyLines(file);
                    }
   
                    if ((*token)->type == TOKEN_RIGHT_PAREN) {
                        break;  // end of args
                    }
                    
                    if ((*token)->type == TOKEN_COMMA) {
                        *token = getTokenAndSkipEmptyLines(file);
                    }

                    if ((*token)->type == TOKEN_KEYWORD && (*token)->param && strcmp((*token)->param->str, "Ifj") == 0) {
                        parseBuiltinCall(file, &args[i++], token);
                    } else {
                        parseExpr(file, &args[i++], token);
                    }

                } while ((*token)->type == TOKEN_COMMA);

                ((BuiltinCall*)cmdItself)->argsCount = i;
                ((BuiltinCall*)cmdItself)->args = args;

                if ((*token)->type != TOKEN_RIGHT_PAREN) {
                    fprintf(stderr, "Syntax error: Expected RIGHT_PAREN got %s.\n", token2String((*token)->type));
                    exit(SYNTAX_ERROR);
                }

                *token = getToken(file);

                if ((*token)->type != TOKEN_NEWLINE) {
                    fprintf(stderr, "Syntax error: Expected NEWLINE got %s.\n", token2String((*token)->type));
                    exit(SYNTAX_ERROR);
                }

                *token = getTokenAndSkipEmptyLines(file);
            }
            // invalid keyword
            else {
                fprintf(stderr, "Syntax error: Invalid statement %s.\n", (*token)->param->str);
                exit(SYNTAX_ERROR);
            }

        }
        else if ((*token)->type == TOKEN_IDENTIFIER) {
            String* id = (*token)->param;

            *token = getToken(file);
            
            // assign statement
            if ((*token)->type == TOKEN_ASSIGN) {
                cmd->type = assign;
                cmdItself = malloc(sizeof(Assign));
                if (cmdItself == NULL) {
                    fprintf(stderr, "Internal error: Memory allocation failed.\n");
                    exit(INTERNAL_ERROR);
                }
                ((Assign*)cmdItself)->expr = NULL;
                ((Assign*)cmdItself)->id = id;
                cmd->cmd = cmdItself;
                
                *token = getToken(file);
                if ((*token)->type == TOKEN_KEYWORD && (*token)->param && strcmp((*token)->param->str, "Ifj") == 0) {
                    parseBuiltinCall(file, &((Assign*)cmdItself)->expr, token);
                } else {
                    parseExpr(file, &((Assign*)cmdItself)->expr, token);
                }
                
                *token = getTokenAndSkipEmptyLines(file);
            }
            // function call
            else if ((*token)->type == TOKEN_LEFT_PAREN) {
                cmd->type = funcCall;
                cmdItself = malloc(sizeof(FuncCall));
                if (cmdItself == NULL) {
                    fprintf(stderr, "Internal error: Memory allocation failed.\n");
                    exit(INTERNAL_ERROR);
                }
                ((FuncCall*)cmdItself)->args = NULL;
                ((FuncCall*)cmdItself)->id = id;
                ((FuncCall*)cmdItself)->argsCount = 0;
                cmd->cmd = cmdItself;
                Expr** args = malloc (sizeof(Expr) * 3);
                if (args == NULL) {
                    fprintf(stderr, "Internal error: Memory allocation failed.\n");
                    exit(INTERNAL_ERROR);
                }
                int i = 0;

                do {                    
                    if ((*token)->type != TOKEN_COMMA) {
                        *token = getTokenAndSkipEmptyLines(file);
                    }
   
                    if ((*token)->type == TOKEN_RIGHT_PAREN) {
                        break;  // end of args
                    }
                    
                    if ((*token)->type == TOKEN_COMMA) {
                        *token = getTokenAndSkipEmptyLines(file);
                    }

                    if ((*token)->type == TOKEN_KEYWORD && (*token)->param && strcmp((*token)->param->str, "Ifj") == 0) {
                        parseBuiltinCall(file, &args[i++], token);
                    } else {
                        parseExpr(file, &args[i++], token);
                    }

                } while ((*token)->type == TOKEN_COMMA);

                ((FuncCall*)cmdItself)->argsCount = i;
                ((FuncCall*)cmdItself)->args = args;

                if ((*token)->type != TOKEN_RIGHT_PAREN) {
                    fprintf(stderr, "Syntax error: Expected RIGHT_PAREN got %s.\n", token2String((*token)->type));
                    exit(SYNTAX_ERROR);
                }

                *token = getToken(file);

                if ((*token)->type != TOKEN_NEWLINE) {
                    fprintf(stderr, "Syntax error: Expected NEWLINE got %s.\n", token2String((*token)->type));
                    exit(SYNTAX_ERROR);
                }

                *token = getTokenAndSkipEmptyLines(file);
            }
            else {
                fprintf(stderr, "Syntax error: Expected ASSIGN or LEFT_PAREN got %s.\n", token2String((*token)->type));
                exit(SYNTAX_ERROR);
            }
        }
        // nested block
        else if ((*token)->type == TOKEN_LEFT_BRACE) {
            cmd->type = block;
            Body* body = NULL;
            *token = getToken(file);
            if ((*token)->type != TOKEN_NEWLINE) {
                fprintf(stderr, "Syntax error: Expected NEWLINE or LEFT_PAREN got %s.\n", token2String((*token)->type));
                exit(SYNTAX_ERROR);
            } 
            parseBody(file, &body, token);
            cmd->cmd = body;

            if ((*token)->type != TOKEN_RIGHT_BRACE) {
                fprintf(stderr, "Syntax error: Expected RIGHT_BRACE got %s.\n", token2String((*token)->type));
                exit(SYNTAX_ERROR);
            }

            *token = getTokenAndSkipEmptyLines(file);
        }
        else if ((*token)->type == TOKEN_NEWLINE) {
            *token = getTokenAndSkipEmptyLines(file);
        }
        else {
            fprintf(stderr, "Syntax error: Invalid statement %s.\n", token2String((*token)->type));
            exit(SYNTAX_ERROR);
        }
    }        
}

void parseCode(FILE* file, Code** ast, Token** token) {
    *token = getTokenAndSkipEmptyLines(file);
    Code** last = ast;
    while ((*token)->type == TOKEN_KEYWORD  && strcmp((*token)->param->str, "static") == 0) {
        Code* globCmd = malloc(sizeof(Code));
        if (globCmd == NULL) {
            fprintf(stderr, "Internal error: Memory allocation failed.\n");
            exit(INTERNAL_ERROR);
        }
        void* globCmdItself = NULL;
        String* id = NULL;
        String** params = malloc(sizeof(String) * MAX_PARAMS_COUNT);
        if (params == NULL) {
            fprintf(stderr, "Internal error: Memory allocation failed.\n");
            exit(INTERNAL_ERROR);
        }
        Body* body = NULL;
        int i = 0;
        
        globCmd->nextGlobalCmd = NULL;

        if (*last != NULL) {
            (*last)->nextGlobalCmd = globCmd;
        } else {
            *last = globCmd;
        }
        last = &globCmd->nextGlobalCmd;
        
        *token = getToken(file);
        if ((*token)->type != TOKEN_IDENTIFIER) {
            fprintf(stderr, "Syntax error: Expected IDENTIFIER got %s.\n", token2String((*token)->type));
            exit(SYNTAX_ERROR);
        }

        id = (*token)->param;

        *token = getToken(file);

        bool isSetter = false;
        bool isGetter = false;
        globCmd->type = funcDecl;
        
        if ((*token)->type != TOKEN_LEFT_BRACE) {
            if ((*token)->type == TOKEN_ASSIGN) {
                // setter
                globCmd->type = setter;
                globCmdItself = malloc(sizeof(Setter));
                if (globCmdItself == NULL) {
                    fprintf(stderr, "Internal error: Memory allocation failed.\n");
                    exit(INTERNAL_ERROR);
                }
                ((Setter*)globCmdItself)->id = id;
                ((Setter*)globCmdItself)->body = NULL;
                globCmd->globCmd = globCmdItself;
                isSetter = true;
                *token = getToken(file);
            } else {
                globCmdItself = malloc(sizeof(FuncDecl));
                if (globCmdItself == NULL) {
                    fprintf(stderr, "Internal error: Memory allocation failed.\n");
                    exit(INTERNAL_ERROR);
                }
                ((FuncDecl*)globCmdItself)->id = id;
                ((FuncDecl*)globCmdItself)->body = NULL;
                globCmd->globCmd = globCmdItself;
            }
            
            if ((*token)->type != TOKEN_LEFT_PAREN) {
                fprintf(stderr, "Syntax error: Expected LEFT_PAREN got %s.\n", token2String((*token)->type));
                exit(SYNTAX_ERROR);
            }
            
            do {
                if ((*token)->type != TOKEN_COMMA) {
                    *token = getTokenAndSkipEmptyLines(file);
                }
                
                if ((*token)->type == TOKEN_RIGHT_PAREN) {
                    break;  // end of params
                }
                
                if ((*token)->type == TOKEN_COMMA) {
                    *token = getTokenAndSkipEmptyLines(file);
                }
                
                if ((*token)->type != TOKEN_IDENTIFIER) {
                    fprintf(stderr, "Syntax error: Expected IDENTIFIER got %s.\n", token2String((*token)->type));
                    exit(SYNTAX_ERROR);
                }
                
                if (i == MAX_PARAMS_COUNT) {
                    fprintf(stderr, "Internal error: Too many params.\n");
                    exit(INTERNAL_ERROR);
                }
                params[i++] = (*token)->param;
                
                *token = getToken(file);
                
            } while ((*token)->type == TOKEN_COMMA && !isSetter);
            
            if (isSetter) {
                ((Setter*)globCmdItself)->param = params[0];
            } else {
                ((FuncDecl*)globCmdItself)->params = params;
                ((FuncDecl*)globCmdItself)->paramsCount = i;
            }
            
            if ((*token)->type != TOKEN_RIGHT_PAREN) {
                fprintf(stderr, "Syntax error: Expected RIGHT_PAREN got %s.\n", token2String((*token)->type));
                exit(SYNTAX_ERROR);
            }
            *token = getTokenAndSkipEmptyLines(file);
        }
        else {
            // getter
            isGetter = true;
            globCmd->type = getter;
            globCmdItself = malloc(sizeof(Getter));
            if (globCmdItself == NULL) {
                fprintf(stderr, "Internal error: Memory allocation failed.\n");
                exit(INTERNAL_ERROR);
            }
            ((Getter*)globCmdItself)->id = id;
            ((Getter*)globCmdItself)->body = NULL;
            globCmd->globCmd = globCmdItself;
        }

        if ((*token)->type != TOKEN_LEFT_BRACE) {
            fprintf(stderr, "Syntax error: Expected LEFT_BRACE got %s.\n", token2String((*token)->type));
            exit(SYNTAX_ERROR);
        }

        parseBody(file, &body, token);
        if (isGetter) {
            ((Getter*)globCmdItself)->body = body;
        } else if (isSetter) {
            ((Setter*)globCmdItself)->body = body;
        } else {
            ((FuncDecl*)globCmdItself)->body = body;
        }

        if ((*token)->type != TOKEN_RIGHT_BRACE) {
            fprintf(stderr, "Syntax error: Expected RIGHT_BRACE got %s.\n", token2String((*token)->type));
            exit(SYNTAX_ERROR);
        }

        *token = getToken(file);

        if ((*token)->type != TOKEN_NEWLINE) {
            fprintf(stderr, "Syntax error: Expected NEWLINE got %s.\n", token2String((*token)->type));
            exit(SYNTAX_ERROR);
        }

        *token = getTokenAndSkipEmptyLines(file);
    }
}

Code* getAST(FILE* file) {
    Code* ast = NULL;
    Token* token = getTokenAndSkipEmptyLines(file);
    if (token->type != TOKEN_IDENTIFIER || strcmp(token->param->str, "import")) {
        fprintf(stderr, "Syntax error: Import statement is invalid.\n");
        exit(SYNTAX_ERROR);
    }
    
    token = getToken(file);

    token = getToken(file);
    if (token->type != TOKEN_IDENTIFIER || strcmp(token->param->str, "for")) {
        fprintf(stderr, "Syntax error: Import statement is invalid.\n");
        exit(SYNTAX_ERROR);
    }

    token = getToken(file);
    if (token->type != TOKEN_KEYWORD || strcmp(token->param->str, "Ifj")) {
        fprintf(stderr, "Syntax error: Import statement is invalid.\n");
        exit(SYNTAX_ERROR);
    }

    token = getTokenAndSkipEmptyLines(file);
    if (token->type != TOKEN_KEYWORD || strcmp(token->param->str, "class")) {
        fprintf(stderr, "Syntax error: Expected CLASS got %s.\n", token2String(token->type));
        exit(SYNTAX_ERROR);
    }

    token = getToken(file);
    if (token->type != TOKEN_IDENTIFIER || strcmp(token->param->str, "Program")) {
        fprintf(stderr, "Syntax error: Expected IDENTIIFIER got %s.\n", token2String(token->type));
        exit(SYNTAX_ERROR);
    }

    token = getTokenAndSkipEmptyLines(file);
    if (token->type != TOKEN_LEFT_BRACE) {
        fprintf(stderr, "Syntax error: Expected LEFT_BRACE got %s.\n", token2String(token->type));
        exit(SYNTAX_ERROR);
    }

    parseCode(file, &ast, &token);

    if (token->type != TOKEN_RIGHT_BRACE) {
        fprintf(stderr, "Syntax error: Expected RIGHT_BRACE got %s.\n", token2String(token->type));
        exit(SYNTAX_ERROR);
    }

    token = getTokenAndSkipEmptyLines(file);
    if (token->type != TOKEN_EOF) {
        fprintf(stderr, "Syntax error: Expected EOF got %s.\n", token2String(token->type));
        exit(SYNTAX_ERROR);
    }

    return ast;
}