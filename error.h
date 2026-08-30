/***************************************************
 * Project: Implementation of the IFJ25 language compiler
 * File   : error.h
 * Author : Milan Babuljak (xbabulm00)
 * Date   : 2. 12. 2025
 *
 * Description: Header file for error codes
 ***************************************************/


#ifndef ERROR_H
#define ERROR_H

// Return code lists

typedef enum {
    SUCCESS = 0,
    LEXICAL_ERROR = 1,
    SYNTAX_ERROR = 2,
    SEMANTIC_ERROR_UNDEFINED = 3,
    SEMANTIC_ERROR_REDEFINITION = 4,
    SEMANTIC_ERROR_FUNCTION_CALL = 5,
    SEMANTIC_ERROR_TYPE_COMPATIBILITY = 6,
    OTHER_SEMANTIC_ERROR = 10,
    RUNTIME_ERROR_WRONG_TYPE = 25,     
    RUNTIME_ERROR_TYPE_COMPATIBILITY = 26, 
    INTERNAL_ERROR = 99 

} ErrorCode;

#endif // ERROR_H