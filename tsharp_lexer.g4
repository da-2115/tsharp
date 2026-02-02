// tsharp_lexer.g4
// Dylan Armstrong, 2026

lexer grammar tsharp_lexer;

WS : [ \t\r\n]+ -> skip ;
INT: 'int';
STRING: 'string';
PRINTLN: 'println';
PRINT: 'print';
ID: [a-zA-Z] [a-zA-Z0-9]*;
NUM: [0-9]+;
STRING_LIT: '"' ~["]* '"';
EQUALS: '=';
OPEN_BRACKET: '(';
CLOSE_BRACKET: ')';
