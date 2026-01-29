// tsharp_parser.g4
// Dylan Armstrong, 2026

parser grammar tsharp_parser;

// Add tsharp lexer to grammar
options {
    tokenVocab = tsharp_lexer;
}

my_test: (TEST)*;