// tsharp_lexer.g4
// Dylan Armstrong, 2026

lexer grammar tsharp_lexer;

WS : [ \t\r\n]+ -> skip ;
INT: 'int';
FLOAT: 'float';
STRING: 'string';
VOID: 'void';
MAIN: 'main';
PRINTLN: 'println';
PRINT: 'print';
ABS: 'abs';
SQRT: 'sqrt';
EXP: 'exp';
PI: 'pi';
RETURN: 'return';
COMMA: ',';
PLUS: '+';
MINUS: '-';
ID: [a-zA-Z] [a-zA-Z0-9]*;
NUM: [-]?[0-9]+;
FLOAT_LIT: NUM+ ([.,] NUM+)?'f' ;
STRING_LIT: '"' ~["]* '"';
EQUALS: '=';
OPEN_BRACKET: '(';
CLOSE_BRACKET: ')';
OPEN_CURLY_BRACE: '{';
CLOSE_CURLY_BRACE: '}';