/* 
Variables.h

The T# Programming Language
Copyright Dylan Armstrong Research and Development 2025
*/

#pragma once

#include <stdbool.h>

typedef struct
{
    char* name;
    int value;
} Int;

typedef struct
{
    char* name;
    float value;
} Float;

typedef struct
{
    char* name;
    double value;
} Double;

typedef struct
{
    char* name;
    char* value;
} String;

typedef struct
{
    char* name;
    char value;
} Char;

typedef struct
{
    char* name;
    bool value;
} Bool;

typedef struct
{
    char* name;
    long value;
} Long;

typedef struct
{
    char* name;
    short value;
} Short;

Int addInt( char* name, int value );
Float addFloat( char* name, float value );
Double addDouble( char* name, double value );
String addString( char* name, char* value );
Char addChar( char* name, char value );
Bool addBool( char* name, bool value );
Long addLong( char* name, long value );
Short addShort( char* name, short value );