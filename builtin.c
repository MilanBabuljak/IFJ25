/***************************************************
 * Project: Implementation of the IFJ25 language compiler
 * File   : builtin.c
 * Author : Adam Bisa (xbisaad00)
 *        : Milan Babuljak (xbabulm00)
 * Date   : 2. 12. 2025
 *
 * Description: Implementaton of the builtin functions
 ***************************************************/

#include "builtin.h"

#include <stdio.h>

void pasteStartInstructionFirst() {
    printf(
        ".IFJcode25\n"
        "CREATEFRAME\n"
    );
}

void pasteStartInstructionSec() {
    printf(
        "CALL $main$$0\n"
        "JUMP $$end\n"
    );
}



void readStringInstruction() {
    printf(
        "LABEL $read_str\n"
        "CREATEFRAME\n"
        "PUSHFRAME\n"
        "CREATEFRAME\n"
        "DEFVAR TF@value\n"
        "MOVE TF@value nil@nil\n"
        "READ TF@value string\n"
        "PUSHS TF@value\n"
        "POPFRAME\n"
        "RETURN\n"
    );
}

void readNumInstruction() {
    printf(
        "LABEL $read_num\n"
        "CREATEFRAME\n"
        "PUSHFRAME\n"
        "CREATEFRAME\n"
        "DEFVAR TF@number\n"
        "READ TF@number float\n" // tu float
        "DEFVAR TF@type\n"
        "ISINT TF@type TF@number\n"
        "JUMPIFEQ $convert_to_int TF@type bool@true\n"
        "PUSHS TF@number\n"
        "POPFRAME\n"
        "RETURN\n"
        "LABEL $convert_to_int\n"
        "FLOAT2INT TF@number TF@number\n"
        "PUSHS TF@number\n"
        "POPFRAME\n"
        "RETURN\n"
    );
}

void writeNumInstruction() {
    printf(
        "LABEL $write_num\n"
        "CREATEFRAME\n"
        "PUSHFRAME\n"
        "CREATEFRAME\n"
        "DEFVAR TF@term\n"
        "DEFVAR TF@is_int\n"
        "POPS TF@term\n"
        "ISINT TF@is_int TF@term\n"     
        "JUMPIFEQ $write_int TF@is_int bool@true\n"
        "WRITE TF@term\n"
        "POPFRAME\n"
        "RETURN\n"
        "LABEL $write_int\n"
        "INT2FLOAT TF@term TF@term\n"
        "WRITE TF@term\n"
        "POPFRAME\n"
        "RETURN\n\n"
    );
}

void readBoolInstruction() {
    printf(
        "LABEL $read_bool\n"
        "CREATEFRAME\n"
        "PUSHFRAME\n"
        "CREATEFRAME\n"
        "DEFVAR TF@value\n"
        "READ TF@value bool\n"
        "CLEARS\n"
        "PUSHS TF@value\n"
        "POPFRAME\n"
        "RETURN\n"
    );
}

void writeInstruction() {
    printf(
        "LABEL $write_str\n"
        "CREATEFRAME\n"
        "PUSHFRAME\n"
        "CREATEFRAME\n"
        "DEFVAR TF@term\n"
        "POPS TF@term\n"
        "WRITE TF@term\n"
        "POPFRAME\n"
        "RETURN\n\n"
    );
}

void floorInstruction() {
    printf(
    "LABEL $floor\n"
    "CREATEFRAME\n"
    "PUSHFRAME\n"
    "CREATEFRAME\n"
    "DEFVAR TF@var\n"
    "DEFVAR TF@type\n"
    "POPS TF@var\n"
    "TYPE TF@type TF@var\n"
    "JUMPIFEQ $floor_end TF@type string@int\n"
    "JUMPIFEQ flr_flt TF@type string@float\n"
    "WRITE string@floor_not_float_or_string\n"
    "EXIT int@25\n"
    "LABEL flr_flt\n"
    "FLOAT2INT TF@var TF@var\n"
    "LABEL $floor_end\n"
    "CLEARS\n"
    "PUSHS TF@var\n"
    "POPFRAME\n"
    "RETURN\n"
);
}

void strInstruction() {
    printf(
        "LABEL $str\n"
        "CREATEFRAME\n"
        "PUSHFRAME\n"
        "CREATEFRAME\n"
        "DEFVAR TF@term\n"
        "POPS TF@term\n"
        "DEFVAR TF@type\n"
        "DEFVAR TF@result\n"
        "TYPE TF@type TF@term\n"
        "JUMPIFEQ $str_int TF@type string@int\n"
        "JUMPIFEQ $str_float TF@type string@float\n"
        "JUMPIFEQ $str_bool TF@type string@bool\n"
        "JUMP $str_end\n"

        "LABEL $str_int\n"
        "INT2STR TF@result TF@term\n"
        "JUMP $str_end\n"

        "LABEL $str_float\n"
        "FLOAT2STR TF@result TF@term\n"
        "JUMP $str_end\n"

        "LABEL $str_bool\n"
        "INT2STR TF@result TF@term\n"

        "LABEL $str_end\n"
        "PUSHS TF@result\n"
        "POPFRAME\n"
        "RETURN\n" 
        "\n"
    );
}

void lenInstruction(){
    printf(
        "LABEL $len\n"
        "CREATEFRAME\n"
        "PUSHFRAME\n"
        "CREATEFRAME\n"
        "DEFVAR TF@literal\n"
        "POPS TF@literal\n"
        "DEFVAR TF@numchars\n"
        "STRLEN TF@numchars TF@literal\n"
        "PUSHS TF@numchars\n"
        "POPFRAME\n"
        "RETURN\n"
        "\n"
    );
}

void chrInstruction(){
    printf(
        "LABEL $chr\n"
        "CREATEFRAME\n"
        "PUSHFRAME\n"
        "CREATEFRAME\n"
        "DEFVAR TF@asc\n"
        "DEFVAR TF@ans\n"
        "POPS TF@asc\n"
        "INT2CHAR TF@ans TF@asc\n"      
        "PUSHS TF@ans\n"
        "POPFRAME\n"
        "RETURN\n"
        "\n"
    );
}

void ordInstruction(){ // ! FUNCTION SIGN. ord(str, index)
    printf(
        "LABEL $ord\n"
        "CREATEFRAME\n"
        "PUSHFRAME\n"
        "CREATEFRAME\n"
        "DEFVAR TF@retazec\n"
        "DEFVAR TF@index\n"
        "POPS TF@index\n"
        "POPS TF@retazec\n"
        "DEFVAR TF@char\n"
        "DEFVAR TF@ascii\n"
        "GETCHAR TF@char TF@retazec TF@index\n"
        "STRI2INT TF@ascii TF@char int@0\n"
        "PUSHS TF@ascii\n"
        "POPFRAME\n"
        "RETURN\n"
        "\n"
    );
}


void substringInstruction(){
    printf(
        "LABEL $substring\n"
        "CREATEFRAME\n"
        "PUSHFRAME\n"
        "CREATEFRAME\n"
        "DEFVAR TF@lit\n"
        "DEFVAR TF@i\n"
        "DEFVAR TF@j\n"
        "DEFVAR TF@output\n"
        "POPS TF@j\n"
        "POPS TF@i\n"
        "POPS TF@lit\n"
        
        // Initialize output to empty string
        "MOVE TF@output string@\n"
        
        "DEFVAR TF@relation\n"
        "DEFVAR TF@len\n"
        "STRLEN TF@len TF@lit\n"
        
        // Check bounds: i >= 0, j >= 0, i <= len, j <= len
        "LT TF@relation TF@i int@0\n"
        "JUMPIFEQ $error TF@relation bool@true\n"
        "LT TF@relation TF@j int@0\n"
        "JUMPIFEQ $error TF@relation bool@true\n"
        "GT TF@relation TF@i TF@len\n"
        "JUMPIFEQ $error TF@relation bool@true\n"
        "GT TF@relation TF@j TF@len\n"
        "JUMPIFEQ $error TF@relation bool@true\n"
        
        // Check if i >= j (empty substring)
        "JUMPIFEQ $empty TF@i TF@j\n"
        "GT TF@relation TF@i TF@j\n"
        "JUMPIFEQ $empty TF@relation bool@true\n"
        
        // Extract substring from i to j
        "DEFVAR TF@char\n"
        "LABEL loop\n"
        "GETCHAR TF@char TF@lit TF@i\n"
        "CONCAT TF@output TF@output TF@char\n"
        "ADD TF@i TF@i int@1\n"
        "JUMPIFNEQ loop TF@i TF@j\n"
        "JUMP $done\n"
        
        "LABEL $empty\n"
        "MOVE TF@output string@\n"
        "JUMP $done\n"
        
        "LABEL $error\n"
        "MOVE TF@output string@\n"
        
        "LABEL $done\n"
        "PUSHS TF@output\n"
        "POPFRAME\n"
        "RETURN\n"
        "\n"
    );
}


void strcmpInstruction(){

    printf(
        "LABEL $strcmp\n"
        "CREATEFRAME\n"
        "PUSHFRAME\n"
        "CREATEFRAME\n"
        "DEFVAR TF@str1\n"
        "DEFVAR TF@str2\n"
        "POPS TF@str2\n"
        "POPS TF@str1\n"
        
        "DEFVAR TF@result\n" 
        
        "DEFVAR TF@len1\n"
        "STRLEN TF@len1 TF@str1\n"
        "DEFVAR TF@len2\n"
        "STRLEN TF@len2 TF@str2\n"
        
        "DEFVAR TF@min_len\n"
        "MOVE TF@min_len TF@len1\n"
        "GT TF@result TF@len1 TF@len2\n"
        "JUMPIFEQ set_len2_min TF@result bool@true\n"
        "JUMP start_loop\n"
        "LABEL set_len2_min\n"
        "MOVE TF@min_len TF@len2\n"
        
        "LABEL start_loop\n"
        "DEFVAR TF@chr1\n"
        "DEFVAR TF@chr2\n"
        "DEFVAR TF@equals\n"
        "DEFVAR TF@c\n"
        "MOVE TF@c int@0\n"
        
        "LABEL for\n"
        "JUMPIFEQ end_loop TF@c TF@min_len\n" 
        
        "GETCHAR TF@chr1 TF@str1 TF@c\n" 
        "GETCHAR TF@chr2 TF@str2 TF@c\n"
        
        "GT TF@result TF@chr1 TF@chr2\n"
        "JUMPIFEQ one_bigger TF@result bool@true\n"
        "LT TF@result TF@chr1 TF@chr2\n"
        "JUMPIFEQ one_smaller TF@result bool@true\n"
        
        "ADD TF@c TF@c int@1\n"
        "JUMP for\n" 
        
        "LABEL end_loop\n"
        "GT TF@result TF@len1 TF@len2\n"
        "JUMPIFEQ one_bigger TF@result bool@true\n"
        "LT TF@result TF@len1 TF@len2\n"
        "JUMPIFEQ one_smaller TF@result bool@true\n"
        
        "MOVE TF@equals int@0\n"
        "JUMP exit\n"

        "LABEL one_smaller\n"
        "MOVE TF@equals int@-1\n"
        "JUMP exit\n"

        "LABEL one_bigger\n"
        "MOVE TF@equals int@1\n"
        "LABEL exit\n"
        "PUSHS TF@equals\n"
        "POPFRAME\n"
        "RETURN\n"
        "\n"
    );

}

void generateBuiltinFunctions(){
    // pasteStartInstruction(); // * WORKS

    readStringInstruction(); // * WORKS
    readBoolInstruction(); // * WORKS
    readNumInstruction(); // * WORKS
    writeNumInstruction(); // * WORKS
    floorInstruction();  // * WORKS
    strInstruction();  // * WORKS
    lenInstruction();  // * WORKS
    chrInstruction(); // * WORKS
    ordInstruction(); // * WROKS
    substringInstruction(); // * WORKS
    strcmpInstruction(); // * WORKS
}
