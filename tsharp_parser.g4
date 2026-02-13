// tsharp_parser.g4
// Dylan Armstrong, 2026

parser grammar tsharp_parser;

// Add tsharp lexer to grammar
options {
    tokenVocab = tsharp_lexer;
}

// The program is the root node for the AST
program: (class)* (function)* main_function EOF;

// Functions
main_function: VOID MAIN OPEN_BRACKET CLOSE_BRACKET OPEN_CURLY_BRACE ( println_statement | print_statement | assignment | absolute_value | square_root | exponent_fn | func_call | expression | object_inst)* CLOSE_CURLY_BRACE;
function: (PUBLIC|PRIVATE) TYPE=type NAME=ID OPEN_BRACKET (ARGS+=function_arg)* CLOSE_BRACKET OPEN_CURLY_BRACE BODY=func_body CLOSE_CURLY_BRACE;
function_arg: TYPE=type NAME=ID;
func_body: (println_statement | print_statement | assignment | absolute_value | square_root | exponent_fn | expression)* return_statement;
func_call: NAME=ID OPEN_BRACKET (func_call_arg)* CLOSE_BRACKET;
func_call_arg: VALUE=(ID|FLOAT_LIT|STRING_LIT|NUM|PI);
return_statement: RETURN VAL=(ID|NUM|FLOAT_LIT|PI);

// OOP
class: CLASS NAME=ID OPEN_CURLY_BRACE (FIELDS+=field)* (CONSTRUCTORS+=constructor)+ (METHODS+=function)* CLOSE_CURLY_BRACE;
field: ACCESS_IDENTIFIER=(PUBLIC|PRIVATE) TYPE=type NAME=ID;
constructor: ACCESS_IDENTIFIER=(PUBLIC|PRIVATE) NAME=ID OPEN_BRACKET (ARGS+=function_arg)* CLOSE_BRACKET OPEN_CURLY_BRACE (constructor_body)? CLOSE_CURLY_BRACE;
constructor_body: (println_statement | print_statement | assignment | absolute_value | square_root | exponent_fn | expression)*;
object_inst: NAME=ID VAR=ID EQUALS OPEN_BRACKET (ARGS+=func_call_arg)* CLOSE_BRACKET;
// A function is a method in terms of nodes in the AST (parser rules)

// Variable assignment
assignment: TYPE=type NAME=ID EQUALS (VALUE=(STRING_LIT|NUM|FLOAT_LIT|PI)|SQUARE_ROOT=square_root|absolute_value);
expression: (THIS DOT)? NAME=ID EQUALS VAR=ID OP=(PLUS|MINUS) (VALUE=(STRING_LIT|NUM|FLOAT_LIT|PI)|(square_root|absolute_value));

// Types
type: (INT|STRING|FLOAT|VOID);

// Math constants
pi_constant: PI;

// Math functions
absolute_value: ABS OPEN_BRACKET (NUMBER=(NUM|FLOAT_LIT)|VAR=ID) CLOSE_BRACKET;
square_root: SQRT OPEN_BRACKET (NUMBER=(NUM|FLOAT_LIT)|VAR=ID) CLOSE_BRACKET;
exponent_fn: EXP OPEN_BRACKET (NUMBER_BASE=NUM|VAR_BASE=ID) COMMA (NUMBER_EXP=NUM|VAR_EXP=ID) CLOSE_BRACKET;

// Methods for printing to screen (println, print)
println_statement: PRINTLN OPEN_BRACKET (MESSAGE=STRING_LIT|VAR=ID|SQUARE_ROOT=square_root|absolute_value|PI_CONST=PI | func_call | (OBJ_NAME=ID DOT PROPERTY_NAME=ID OPEN_BRACKET (ARGS+=func_call_arg)* CLOSE_BRACKET)) CLOSE_BRACKET;
print_statement: PRINT OPEN_BRACKET (MESSAGE=STRING_LIT|VAR=ID|SQUARE_ROOT=square_root|absolute_value|PI_CONST=PI | func_call) CLOSE_BRACKET;
