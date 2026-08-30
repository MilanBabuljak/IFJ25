/***************************************************
 * Project: Implementation of the IFJ25 language compiler
 * File   : builtin.h
 * Author : Adam Bisa (xbisaad00)
 *        : Milan Babuljak (xbabulm00)
 * Date   : 2. 12. 2025
 *
 * Description: Header file for builtin functions
 ***************************************************/


#ifndef BUILTIN_H
#define BUILTIN_H


void pasteStartInstructionFirst();
void pasteStartInstructionSec();

void readStringInstruction();
void readNumInstruction();
void readBoolInstruction();

void writeInstruction();

void floorInstruction();
void strInstruction();
void lenInstruction();
void chrInstruction();
void ordInstruction();
void substringInstruction();
void strcmpInstruction();

void generateBuiltinFunctions(); 

// * above are implemented && tested




#endif // BUILTIN_H
