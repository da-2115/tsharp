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

extern const char* operators[];
extern const char* keywords[];