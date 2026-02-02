// tsharp_parser.g4
// Dylan Armstrong, 2026

parser grammar tsharp_parser;

// Add tsharp lexer to grammar
options {
    tokenVocab = tsharp_lexer;
}

// The program is the root node for AST
program: (println_statement | print_statement | assignment)* EOF;

// Variable assignment
assignment: TYPE=type NAME=ID EQUALS VALUE=(STRING_LIT|NUM);

// Types
type: (INT|STRING);

// Methods for printing to screen (println, print)
println_statement: PRINTLN OPEN_BRACKET (MESSAGE=STRING_LIT|VAR=ID) CLOSE_BRACKET;
print_statement: PRINT OPEN_BRACKET (MESSAGE=STRING_LIT|VAR=ID) CLOSE_BRACKET;
