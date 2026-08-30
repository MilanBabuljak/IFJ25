/***************************************************
 * Project: Implementation of the IFJ25 language compiler
 * File   : string.h
 * Author : Milan Babuljak (xbabulm00)
 * Date   : 2. 12. 2025
 *
 * Description: Header file for String
 ***************************************************/


#ifndef STRING_H
#define STRING_H

#include <stdlib.h>

typedef struct {
    char *str;
    int length;
    int capacity;
} String;


String* stringInit();
void stringFree(String* s);
int stringConcatCstr(String* s, char* cstr);
char* stringGetCStr(String* s);

int stringGetLength(String* s);
String* makeS(const char *cstr);
void stringClear(String* s);
void stringAppendChar(String* s, char c);
int stringSet(String* s, char* cstr);
void stringPopBack(String* s);
int stringCompare(String* s1, String* s2);

#endif // STRING_H