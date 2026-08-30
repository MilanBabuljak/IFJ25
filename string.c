/***************************************************
 * Project: Implementation of the IFJ25 language compiler
 * File   : string.c
 * Author : Milan Babuljak (xbabulm00)
 * Date   : 2. 12. 2025
 *
 * Description: String implementation
 ***************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "string.h"
#include "error.h"


// Init of empty string
String* stringInit() {
    String* s = malloc(sizeof(String));
    if (s == NULL) {
        fprintf(stderr, "Internal error: Memory allocation failed.\n");
        exit(INTERNAL_ERROR);
    }

    s->length = 0;
    s->capacity = 1;   
    s->str = malloc(s->capacity);
    if ( s->str == NULL) {
        fprintf(stderr, "Internal error: Memory allocation failed.\n");
        exit(INTERNAL_ERROR);
    }

    s->str[0] = '\0';
    return s;
}

void stringPopBack(String* s) {
    if (!s || s->length == 0) return;
    s->length--;
    s->str[s->length] = '\0';
}


// Appending single character to string
void stringAppendChar(String* s, char c) {
    if (!s) return;

    if (s->length + 1 >= s->capacity) {
        int newCapacity = s->capacity * 2;
        if (newCapacity < s->length + 2) {
            newCapacity = s->length + 2;  // ensure enough space
        }
        char* newStr = realloc(s->str, newCapacity);
        if (!newStr) return;  // allocation failed
        s->str = newStr;
        s->capacity = newCapacity;
    }
    s->str[s->length++] = c;
    s->str[s->length] = '\0';
}

// Concatination of C-string to String
int stringConcatCstr(String* s, char* cstr) {
    if (!s || !cstr) return INTERNAL_ERROR;
    int cstrLen = strlen(cstr);

    if (s->length + cstrLen + 1 > s->capacity) {
        int newCapacity = (s->length + cstrLen + 1) * 2;
        char* newStr = realloc(s->str, newCapacity);
        if (!newStr) return INTERNAL_ERROR;
        s->str = newStr;
        s->capacity = newCapacity;
    }

    memcpy(s->str + s->length, cstr, cstrLen + 1);
    s->length += cstrLen;
    return 0;
}

    
void stringClear(String* s) {
    if (s) {
        s->length = 0;
        if (s->capacity > 0) {
            s->str[0] = '\0';
        }
    }
}

// Compare two strings
int stringCompare(String* s1, String* s2) {
    if (!s1 || !s2) return -2;
    return strcmp(s1->str, s2->str);
}

// Create String from C-"string" 
String* makeS(const char *cstr) {
    String *s = stringInit();
    if (!s) return NULL;
    stringSet(s, (char*)cstr);
    return s;
}

// Get C-"string" from String
char* stringGetCStr(String* s) {
    return (s && s->str) ? s->str : "";
}

int stringGetLength(String* s) {
    return s ? s->length : 0;
}

// Set String to C-"string"
int stringSet(String* s, char* cstr) {
    if (!s || !cstr) return INTERNAL_ERROR;
    stringClear(s);
    return stringConcatCstr(s, cstr);
}
