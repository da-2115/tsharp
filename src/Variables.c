/* 
Variable.c

The T# Programming Language
Copyright Dylan Armstrong Research and Development 2025
*/

#include "Variables.h"

#include <string.h>

Int addInt(char* name, int value)
{
    Int i;
    i.name = strdup(name);
    i.value = value;

    return i;
}

Float addFloat( char* name, float value )
{
    Float f;
    f.name = name;
    f.value = value;

    return f;
}

Double addDouble( char* name, double value )
{
    Double d;
    d.name = name;
    d.value = value;

    return d;
}

String addString( char* name, char* value )
{
    String s;
    s.name = name;
    s.value = value;

    return s;
}

Char addChar( char* name, char value )
{
    Char c;
    c.name = name;
    c.value = value;

    return c;
}

Bool addBool( char* name, bool value )
{
    Bool b;
    b.name = name;
    b.value = value;

    return b;
}

Long addLong( char* name, long value )
{
    Long l;
    l.name = name;
    l.value = value;

    return l;
}

Short addShort( char* name, short value )
{
    Short s;
    s.name = name;
    s.value = value;

    return s;
}