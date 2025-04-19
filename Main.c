/* 
Main.c

The T# Programming Language
Copyright Dylan Armstrong Research and Development 2025
*/

#include "Token.h"
#include "Node.h"
#include "Variables.h"
#include "Program.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int run(Node** nodes, int* numOfNodes)
{
    Program* program = malloc( sizeof( Program ) );
    program->numOfIntegers = 0;
    program->sizeOfIntegers = 10;

    for ( int i = 0; i < *numOfNodes; i++ )
    {
        printf("Node %d: Token %s Token Type: %d\n", i, nodes[i]->token.value, nodes[i]->token.type);

        if ( nodes[i]->token.type == KEYWORD )
        {
            char* variableType = nodes[i]->token.value;
            printf("Variable type: %s\n", variableType);
            if ( nodes[i]->next->token.type == VARNAME )
            {
                char* variableName = nodes[i]->next->token.value;
                printf("Variable name is %s\n", variableName);
                if ( nodes[i]->next->next->token.type == OPERATOR )
                {
                    if ( nodes[i]->next->next->next->token.type == VARVAL )
                    {
                        if ( strcmp(variableType, keywords[1]) == 0 ) // int
                        {
                            int value = atoi(nodes[i]->next->next->next->token.value);
                            printf("Debug: Variable name = %s Value = %d\n", variableName, value);
                            Int i = addInt( variableName, value );
                            addIntToProgram( program, &i );
                        }
                    }
                }
            }
        }
    }

    printf("Variable %s of type int is %d\n", program->integers[0]->name, program->integers[0]->value);

    freeProgram(program);

    return 0;
}

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


    printf("Tokens: %d, Nodes: %d\n", numOfTokens, numOfNodes);
    int result = run( nodes, &numOfNodes );

    free( nodes );
    free( tokens );
    free( sourceCode );

    return result;
}