// tsharp_lexer.g4
// Dylan Armstrong, 2026

lexer grammar tsharp_lexer;

ID: [a-zA-Z] [a-zA-Z0-9]*;
STRING_LIT: '"' ~["]* '"';
OPEN_BRACKET: '(';
CLOSE_BRACKET: ')';
PRINTLN: 'println';
PRINT: 'print';