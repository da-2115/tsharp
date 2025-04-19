/* 
Program.c

The T# Programming Language
Copyright Dylan Armstrong Research and Development 2025
*/

#include "Program.h"

#include <stdlib.h>

void addIntToProgram(Program* program, Int* integer)
{
    if ( program->numOfIntegers == program->sizeOfIntegers )
    {
        program->sizeOfIntegers += 10;
        program->integers = realloc( program->integers, program->sizeOfIntegers * sizeof( Int ) );
    }

    program->integers[program->numOfIntegers] = malloc(sizeof(Int));
    program->integers[program->numOfIntegers]->name = strdup(integer->name);
    program->integers[program->numOfIntegers]->value = integer->value;

    program->numOfIntegers++;
}

void freeProgram(Program* program)
{
    for (int i = 0; i < program->numOfIntegers; i++)
    {
        free(program->integers[i]->name);
        free(program->integers[i]);
    }

    free( program->integers );
    free( program );
}