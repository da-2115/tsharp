/* 
Program.h

The T# Programming Language
Copyright Dylan Armstrong Research and Development 2025
*/

#pragma once

#include "Variables.h"

typedef struct
{
    Int** integers;
    int numOfIntegers;
    int sizeOfIntegers;
} Program;


void addIntToProgram(Program* program, Int* integer);
void freeProgram(Program* program);