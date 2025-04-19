/* 
Token.c

The T# Programming Language
Copyright Dylan Armstrong Research and Development 2025
*/

#include "Token.h"

#include <stdlib.h>
#include <string.h>

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