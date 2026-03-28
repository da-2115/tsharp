grammar TSharp;

// Parser rules

program
    : (NEWLINE | topLevelDecl)* EOF
    ;

topLevelDecl
    : functionDecl
    | classDecl
    | interfaceDecl
    | enumDecl
    | globalVarDecl
    ;

globalVarDecl
    : modifiers? variableDecl NEWLINE+
    ;

modifiers
    : modifier+
    ;

modifier
    : accessModifier
    | STATIC
    | ABSTRACT
    | VIRTUAL
    | OVERRIDE
    ;

accessModifier
    : PUBLIC
    | PRIVATE
    | PROTECTED
    ;

classDecl
    : modifiers? CLASS IDENTIFIER NEWLINE* genericParams? inheritanceClause? classBody
    ;

interfaceDecl
    : modifiers? INTERFACE IDENTIFIER NEWLINE* genericParams? inheritanceClause? interfaceBody
    ;

enumDecl
    : modifiers? ENUM IDENTIFIER NEWLINE* LBRACE NEWLINE* enumMember (COMMA? NEWLINE* enumMember)* COMMA? NEWLINE* RBRACE NEWLINE*
    ;

enumMember
    : IDENTIFIER (ASSIGN INTEGER_LITERAL)?
    ;

inheritanceClause
    : COLON typeRef (COMMA typeRef)*
    ;

genericParams
    : LT IDENTIFIER (COMMA IDENTIFIER)* GT
    ;

typeArgs
    : LT typeRef (COMMA typeRef)* GT
    ;

typeRef
    : primitiveType arrayTypeSuffix?
    | IDENTIFIER typeArgs? arrayTypeSuffix?
    | ANY arrayTypeSuffix?
    ;

arrayTypeSuffix
    : LBRACK RBRACK
    ;

primitiveType
    : INT
    | FLOAT
    | DOUBLE
    | STRING
    | CHAR
    | BOOL
    | VOID
    ;

classBody
    : LBRACE (NEWLINE | classMember)* RBRACE
    ;

interfaceBody
    : LBRACE (NEWLINE | interfaceMember)* RBRACE
    ;

classMember
    : NEWLINE+
    | constructorDecl
    | methodDecl
    | propertyDecl NEWLINE+
    | fieldDecl NEWLINE+
    ;

interfaceMember
    : NEWLINE+
    | interfaceMethodDecl
    | interfacePropertySignature NEWLINE+
    | interfaceFieldSignature NEWLINE+
    ;

fieldDecl
    : modifiers? typeRef IDENTIFIER arrayDeclarator? (ASSIGN expression)?
    ;

propertyDecl
    : modifiers? typeRef IDENTIFIER ARROW expression
    | modifiers? typeRef IDENTIFIER NEWLINE* block
    ;

interfacePropertySignature
    : accessModifier? typeRef IDENTIFIER
    ;

interfaceFieldSignature
    : accessModifier? typeRef IDENTIFIER
    ;

constructorDecl
    : accessModifier? LPAREN parameterList? RPAREN NEWLINE* block
    ;

methodDecl
    : modifiers? returnType IDENTIFIER genericParams? LPAREN parameterList? RPAREN NEWLINE* block
    ;

interfaceMethodDecl
    : accessModifier? returnType IDENTIFIER genericParams? LPAREN parameterList? RPAREN NEWLINE+
    ;

functionDecl
    : modifiers? returnType IDENTIFIER genericParams? LPAREN parameterList? RPAREN NEWLINE* block
    ;

returnType
    : typeRef
    | VOID
    ;

parameterList
    : parameter (COMMA parameter)*
    ;

parameter
    : typeRef IDENTIFIER
    ;

block
    : NEWLINE* LBRACE NEWLINE* statement* RBRACE;

statement
    : NEWLINE+
    | block
    | variableStatement
    | assignmentStatement
    | expressionStatement
    | ifStatement
    | whileStatement
    | doWhileStatement
    | forStatement
    | switchStatement
    | breakStatement
    | continueStatement
    | returnStatement
    | tryStatement
    | throwStatement
    ;

variableStatement
    : modifiers? variableDecl NEWLINE+
    ;

variableDecl
    : typeRef IDENTIFIER arrayDeclarator? variableInitializer?
    ;

variableInitializer
    : ASSIGN expression
    | LPAREN argumentList? RPAREN
    ;

arrayDeclarator
    : LBRACK expression? RBRACK
    ;

assignmentStatement
    : assignment NEWLINE+
    ;

assignment
    : lvalue assignmentOperator expression
    ;

lvalue
    : primaryLValue lvalueSuffix*
    ;

primaryLValue
    : IDENTIFIER
    | THIS
    | BASE
    ;

lvalueSuffix
    : DOT IDENTIFIER
    | LBRACK expression RBRACK
    ;

assignmentOperator
    : ASSIGN
    | ADD_ASSIGN
    | SUB_ASSIGN
    | MUL_ASSIGN
    | DIV_ASSIGN
    | MOD_ASSIGN
    ;

expressionStatement
    : expression NEWLINE+
    ;

ifStatement
    : IF LPAREN expression RPAREN NEWLINE* block (ELSE IF LPAREN expression RPAREN block)* (ELSE block)?
    ;

whileStatement
    : WHILE LPAREN expression RPAREN NEWLINE* block
    ;

doWhileStatement
    : DO NEWLINE* block WHILE LPAREN expression RPAREN NEWLINE+
    ;

// For loop uses semicolons (only T# feature with semicolons)
// for (init; condition; update)
forStatement
    : FOR LPAREN forInit? SEMICOLON expression? SEMICOLON forUpdate? RPAREN NEWLINE* block
    ;

forInit
    : variableDecl
    | assignment
    | expression
    ;

forUpdate
    : assignment (COMMA assignment)*
    | expression (COMMA expression)*
    ;

switchStatement
    : SWITCH LPAREN expression RPAREN LBRACE (switchSection | NEWLINE)* RBRACE
    ;

switchSection
    : (CASE expression COLON | DEFAULT COLON) (NEWLINE | statement)*
    ;

breakStatement
    : BREAK NEWLINE+
    ;

continueStatement
    : CONTINUE NEWLINE+
    ;

returnStatement
    : RETURN expression? NEWLINE+
    ;

tryStatement
    : TRY NEWLINE* block catchClause* finallyClause?
    ;

catchClause
    : CATCH LPAREN typeRef IDENTIFIER RPAREN NEWLINE* block
    ;

finallyClause
    : FINALLY NEWLINE* block
    ;

throwStatement
    : THROW expression NEWLINE+
    ;

expression
    : logicalOrExpression
    ;

logicalOrExpression
    : logicalAndExpression (OR_OR logicalAndExpression)*
    ;

logicalAndExpression
    : equalityExpression (AND_AND equalityExpression)*
    ;

equalityExpression
    : relationalExpression ((EQ | NEQ) relationalExpression)*
    ;

relationalExpression
    : additiveExpression ((LT | LTE | GT | GTE) additiveExpression)*
    ;

additiveExpression
    : multiplicativeExpression ((ADD | SUB) multiplicativeExpression)*
    ;

multiplicativeExpression
    : unaryExpression ((MUL | DIV | MOD) unaryExpression)*
    ;

unaryExpression
    : (NOT | SUB | ADD | INC | DEC) unaryExpression
    | postfixExpression
    ;

postfixExpression
    : primary postfixSuffix*
    ;

postfixSuffix
    : LPAREN argumentList? RPAREN
    | DOT IDENTIFIER
    | LBRACK expression RBRACK
    | INC
    | DEC
    ;

primary
    : literal
    | IDENTIFIER typeArgs?
    | THIS
    | BASE
    | LPAREN expression RPAREN
    | arrayLiteral
    ;

argumentList
    : expression (COMMA expression)*
    ;

arrayLiteral
    : LBRACE (expression (COMMA expression)*)? RBRACE
    ;

literal
    : INTEGER_LITERAL
    | FLOAT_LITERAL
    | STRING_LITERAL
    | CHAR_LITERAL
    | TRUE
    | FALSE
    | NULL
    ;

// Lexer rules

CLASS      : 'class';
INTERFACE  : 'interface';
ENUM       : 'enum';
ABSTRACT   : 'abstract';
STATIC     : 'static';
PUBLIC     : 'public';
PRIVATE    : 'private';
PROTECTED  : 'protected';
VIRTUAL    : 'virtual';
OVERRIDE   : 'override';

IF         : 'if';
ELSE       : 'else';
FOR        : 'for';
WHILE      : 'while';
DO         : 'do';
SWITCH     : 'switch';
CASE       : 'case';
DEFAULT    : 'default';
BREAK      : 'break';
CONTINUE   : 'continue';
RETURN     : 'return';
TRY        : 'try';
CATCH      : 'catch';
THROW      : 'throw';
FINALLY    : 'finally';

THIS       : 'this';
BASE       : 'base';

INT        : 'int';
FLOAT      : 'float';
DOUBLE     : 'double';
STRING     : 'string';
CHAR       : 'char';
BOOL       : 'bool';
VOID       : 'void';
ANY        : 'any';

TRUE       : 'true';
FALSE      : 'false';
NULL       : 'null';

ARROW      : '->';

ADD_ASSIGN : '+=';
SUB_ASSIGN : '-=';
MUL_ASSIGN : '*=';
DIV_ASSIGN : '/=';
MOD_ASSIGN : '%=';

EQ         : '==';
NEQ        : '!=';
LTE        : '<=';
GTE        : '>=';

AND_AND    : '&&';
OR_OR      : '||';

INC        : '++';
DEC        : '--';

ASSIGN     : '=';
ADD        : '+';
SUB        : '-';
MUL        : '*';
DIV        : '/';
MOD        : '%';
NOT        : '!';

LT         : '<';
GT         : '>';

LPAREN     : '(';
RPAREN     : ')';
LBRACE     : '{';
RBRACE     : '}';
LBRACK     : '[';
RBRACK     : ']';

COMMA      : ',';
DOT        : '.';
COLON      : ':';
SEMICOLON  : ';';
PIPE       : '|';

IDENTIFIER
    : [a-zA-Z_][a-zA-Z0-9_]*
    ;

INTEGER_LITERAL
    : [0-9]+
    ;

FLOAT_LITERAL
    : [0-9]+ '.' [0-9]+
    ;

STRING_LITERAL
    : '"' ( '\\' . | ~["\\\r\n] )* '"'
    ;

CHAR_LITERAL
    : '\'' ( '\\' . | ~['\\\r\n] ) '\''
    ;

NEWLINE
    : '\r'? '\n'+
    ;

WS
    : [ \t\f]+ -> skip
    ;

LINE_COMMENT
    : '//' ~[\r\n]* -> skip
    ;

BLOCK_COMMENT
    : '/*' .*? '*/' -> skip
    ;