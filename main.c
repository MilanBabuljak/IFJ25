/***************************************************
 * Project: Implementation of the IFJ25 language compiler
 * File   : main.c
 * Author : Milan Babuljak (xbabulm00)
 *        : Jaroslav Synek (xsynekj00)
 *        : Adam Bisa (xbisaad00)
 * Date   : 2. 12. 2025
 *
 * Description: Main entry point for the IFJ25 compiler
 ***************************************************/


#include <stdio.h>
#include <stdlib.h>
#include "string.h"
#include "lexer.h"
#include "ast.h"
#include "debug_utils.h"
#include "symtable.h"
#include "semtree.h"
#include "ast-visualiser.h"
#include "codegen.h"
#include "semantic.h"
#include <string.h>

void usage(const char *prog) {
    fprintf(stderr,
        "Usage: %s [source_file] [options]\n"
        "Options:\n"
        "  -t, --tokens     Dump tokens from lexer\n"
        "  -v, --verify     Verify tokens while lexing\n"
        "  -a, --ast        Print AST (default if no explicit flags)\n"
        "  -c, --codegen    Run/print codegen (default if no explicit flags)\n"
        "If source_file is omitted, input is read from stdin.\n",
        prog);
}

int main(int argc, char *argv[]) {
    const int useStdin = (argc < 2);
    const char *srcPath = useStdin ? NULL : argv[1];

    int flagTokens  = 0;
    int flagVerify  = 0;
    int flagAST     = 0;
    int flagCodegen = 0;
    int flagDebug   = 0;

    /* parse simple flags */
    for (int i = 2; i < argc; ++i) {
        if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--tokens") == 0) flagTokens = 1;
        else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verify") == 0) flagVerify = 1;
        else if (strcmp(argv[i], "-a") == 0 || strcmp(argv[i], "--ast") == 0) flagAST = 1;
        else if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--codegen") == 0) flagCodegen = 1;
        else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            usage(argv[0]);
            return EXIT_FAILURE;
        }
    }

    if (!flagTokens && !flagVerify && !flagAST && !flagCodegen && !flagDebug) {
        flagCodegen = 1;
    }


    FILE *file = NULL;
    if (useStdin) {
        file = stdin;
    } else {
        file = fopen(srcPath, "r");
        if (!file) {
            perror("Failed to open source file");
            return EXIT_FAILURE;
        }
    }

    if (flagTokens || flagVerify) {
        previousToken.type = TOKEN_EOF;
        previousToken.param = NULL;

        Token *token = NULL;
        do {
            token = getToken(file);
            verifyToken(token);
            if (flagTokens) printToken(token);
        } while (token->type != TOKEN_EOF);

        if (flagAST || flagCodegen) {
            if (fseek(file, 0, SEEK_SET) != 0) {
                perror("Failed to rewind source file for AST/codegen");
                fclose(file);
                return EXIT_FAILURE;
            }
        }
    }

    semtree_node_t *globalScope = NULL;

    if (flagAST || flagCodegen) {
        Code *ast = getAST(file);
        if (!ast) {
            fprintf(stderr, "Error: getAST returned NULL\n");
            fclose(file);
            return EXIT_FAILURE;
        }

        if (flagAST) {
            printAST(ast);
        }

        ErrorCode semResult = SUCCESS;
        globalScope = semanticAnalyze(ast, &semResult);
        if (semResult != SUCCESS) {
            if (!useStdin) {
                fclose(file);
            }
            return semResult;
        }

        if (flagCodegen) {
            codegenPrint(ast, globalScope);
        }

        semtreeFree(globalScope);

    }

    if (!useStdin) {
        fclose(file);
    }
    return EXIT_SUCCESS;
}