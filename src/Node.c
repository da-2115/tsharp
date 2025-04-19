/* 
Node.c

The T# Programming Language
Copyright Dylan Armstrong Research and Development 2025
*/

#include "Node.h"

#include <stdio.h>
#include <stdlib.h>

Node** createNodes( Token* tokens, int* numOfTokens, int* numOfNodes )
{
    *numOfNodes = 0;

    Node** nodes = malloc( *numOfTokens * sizeof( Node* ) );
    if ( !nodes )
    {
        fprintf(stderr, "Memory allocation for nodes failed.\n");
        return NULL;
    }

    for ( int i = 0; i < *numOfTokens; i++ )
    {
        nodes[i] = malloc( sizeof( Node ) );
        if ( !nodes[i] )
        {
            fprintf(stderr, "Memory allocation for node %d failed.\n", i);
            return NULL;
        }

        nodes[i]->token = tokens[i];
        nodes[i]->prev = NULL;
        nodes[i]->next = NULL;
    }

    *numOfNodes = *numOfTokens;

    for ( int i = 0; i < *numOfNodes; i++ )
    {
        nodes[i]->prev = (i > 0) ? nodes[i - 1] : NULL;
        nodes[i]->next = (i < *numOfNodes - 1) ? nodes[i + 1] : NULL;

        if ( nodes[i]->prev != NULL && nodes[i]->prev->token.type == OPERATOR )
        {
            nodes[i]->token.type = VARVAL;
        }

        else if ( nodes[i]->prev != NULL && nodes[i]->prev->token.type == KEYWORD )
        {
            nodes[i]->token.type = VARNAME;
        }
    }

    return nodes;
}