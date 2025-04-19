/* 
Main.c

The T# Programming Language
Copyright Dylan Armstrong Research and Development 2025
*/

#include "Token.h"
#include "Node.h"

#include <stdio.h>
#include <stdlib.h>

int main( int argc, char** argv )
{
    if ( argc < 2 )
    {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        return 1;
    }

    FILE* file = fopen(argv[1], "r");
    if ( !file )
    {
        perror("Failed to open file");
        return 1;
    }

    fseek( file, 0, SEEK_END );
    long fileSize = ftell( file );
    rewind( file );

    char* sourceCode = malloc( fileSize + 1 );
    if ( !sourceCode )
    {
        fprintf(stderr, "Memory allocation for source code failed.\n");
        fclose(file);
        return 1;
    }

    int bytesRead = fread( sourceCode, 1, fileSize, file );
    sourceCode[bytesRead] = '\0';
    fclose(file);

    int numOfTokens = 0;
    Token* tokens = createTokens( sourceCode, &numOfTokens );


    int numOfNodes = 0;
    Node** nodes = createNodes( tokens, &numOfTokens, &numOfNodes );
    
    free( nodes );
    free( tokens );
    free( sourceCode );

    return 0;
}