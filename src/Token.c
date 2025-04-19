/* 
Token.c

The T# Programming Language
Copyright Dylan Armstrong Research and Development 2025
*/

#include "Token.h"

#include <stdlib.h>
#include <string.h>


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
    "private",  // 15
    "case",     // 16
    "con"       // 17
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
    "--",       // 7
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
    "[]",       // 21
    "{",        // 22
    "}",        // 23
    ";",        // 24
    "(",        // 25
    ")",        // 26
    ":"         // 27
};


// Get token type based off word from createTokens function
enum TokenType getType( char* word )
{
    enum TokenType tokenType;

    int numOfKeywords = sizeof( keywords ) / sizeof( keywords[0] );

    for ( int i = 0; i < numOfKeywords; i++ )
    {
        if ( word == keywords[i] )
        {
            tokenType = KEYWORD;
        }
    }

    return tokenType;
}

// Create tokens array from source code
Token* createTokens( char* source, int* numOfTokens )
{
    int size = 10;
    *numOfTokens = 0;

    Token* tokens = malloc( size * sizeof( Token ) );

    char* word = strtok( source, " \t\n" );

    while ( word != NULL )
    {
        // Resize array if the size and number of tokens are equal (increments of 10)
        if ( *numOfTokens == size )
        {
            size += 10;
            tokens = realloc( tokens, size * sizeof( Token ) );
        }

        tokens[*numOfTokens].value = strdup( word );
        tokens[*numOfTokens].type = getType( word );

        (*numOfTokens)++;
        word = strtok( NULL, " \t\n" );
    }

    return tokens;
}
