/* 
Token.h

The T# Programming Language
Copyright Dylan Armstrong Research and Development 2025
*/

#pragma once

enum TokenType
{
    KEYWORD,
    VARNAME,
    FUNCNAME,
    OPERATOR,
    FUNCARG,
    VARVAL
};

typedef struct
{
    enum TokenType type;
    char* value;
} Token;

enum TokenType getType( char* word );
Token* createTokens( char* source, int* numOfTokens );

const char* keywords[] =
{
    "fn",       // 0
    "int",      // 1
    "float",    // 2
    "double",   // 3
    "string",   // 4
    "char",     // 5
    "bool",     // 6
    "long",     // 7
    "short",    // 8
    "return",   // 9
    "class",    // 10
    "enum",     // 11
    "switch",   // 12
    "if",       // 13
    "else",     // 14
    "private"   // 15
};

const char* operators[] = 
{
    "=",        // 0
    "+",        // 1
    "-",        // 2
    "*",        // 3
    "/",        // 4
    "%%",       // 5
    "++",       // 6
    "--"        // 7
    "+=",       // 8
    "-=",       // 9
    "*=",       // 10
    "/=",       // 11
    ">",        // 12
    ">=",       // 13
    "<",        // 14
    "<=",       // 15
    "==",       // 16
    "!=",       // 17
    "!",        // 18
    "&&",       // 19
    "||",       // 20
    "[]"        // 21
    "{"         // 22
    "}",        // 23
    ";"         // 24
    "(",        // 25
    ")"         // 26
};
