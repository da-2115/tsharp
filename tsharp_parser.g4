// tsharp_parser.g4
// Dylan Armstrong, 2026

parser grammar tsharp_parser;

// Add tsharp lexer to grammar
options {
    tokenVocab = tsharp_lexer;
}

// The program is the root node for the AST
program: (println_statement | print_statement | assignment | absolute_value | square_root | exponent_fn)* EOF;

// Variable assignment
assignment: TYPE=type NAME=ID EQUALS (VALUE=(STRING_LIT|NUM)|SQUARE_ROOT=square_root);

// Types
type: (INT|STRING);

// Math functions
absolute_value: ABS OPEN_BRACKET (NUMBER=NUM|VAR=ID) CLOSE_BRACKET;
square_root: SQRT OPEN_BRACKET (NUMBER=NUM|VAR=ID) CLOSE_BRACKET;
exponent_fn: EXP OPEN_BRACKET (NUMBER_BASE=NUM|VAR_BASE=ID) COMMA (NUMBER_EXP=NUM|VAR_EXP=ID) CLOSE_BRACKET;
// Methods for printing to screen (println, print)
println_statement: PRINTLN OPEN_BRACKET (MESSAGE=STRING_LIT|VAR=ID|SQUARE_ROOT=square_root) CLOSE_BRACKET;
print_statement: PRINT OPEN_BRACKET (MESSAGE=STRING_LIT|VAR=ID|SQUARE_ROOT=square_root) CLOSE_BRACKET;
