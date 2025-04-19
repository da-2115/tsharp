/* 
Node.h

The T# Programming Language
Copyright Dylan Armstrong Research and Development 2025
*/

#pragma once

#include "Token.h"

typedef struct Node
{
    struct Node* prev;
    struct Node* next;
    Token token;
} Node;

Node** createNodes( Token* tokens, int* numOfTokens, int* numOfNodes );