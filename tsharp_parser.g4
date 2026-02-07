// tsharp_parser.g4
// Dylan Armstrong, 2026

parser grammar tsharp_parser;

// Add tsharp lexer to grammar
options {
    tokenVocab = tsharp_lexer;
}

// The program is the root node for the AST
program: (function)* main_function EOF;

main_function: VOID MAIN OPEN_BRACKET CLOSE_BRACKET OPEN_CURLY_BRACE (println_statement | print_statement | assignment | absolute_value | square_root | exponent_fn | func_call | expression)* CLOSE_CURLY_BRACE;

function: TYPE=type NAME=ID OPEN_BRACKET (ARGS+=function_arg)* CLOSE_BRACKET OPEN_CURLY_BRACE BODY=func_body CLOSE_CURLY_BRACE;
function_arg: TYPE=type NAME=ID;
func_body: (println_statement | print_statement | assignment | absolute_value | square_root | exponent_fn | expression)* return_statement;
func_call: NAME=ID OPEN_BRACKET CLOSE_BRACKET;

return_statement: RETURN VAL=(ID|NUM|FLOAT_LIT|PI);

// Variable assignment
assignment: TYPE=type NAME=ID EQUALS (VALUE=(STRING_LIT|NUM|FLOAT_LIT|PI)|SQUARE_ROOT=square_root|absolute_value);
expression: NAME=ID EQUALS VAR=ID OP=(PLUS|MINUS) (VALUE=(STRING_LIT|NUM|FLOAT_LIT|PI)|(square_root|absolute_value));

// Types
type: (INT|STRING|FLOAT|VOID);

// Math constants
pi_constant: PI;

// Math functions
absolute_value: ABS OPEN_BRACKET (NUMBER=(NUM|FLOAT_LIT)|VAR=ID) CLOSE_BRACKET;
square_root: SQRT OPEN_BRACKET (NUMBER=(NUM|FLOAT_LIT)|VAR=ID) CLOSE_BRACKET;
exponent_fn: EXP OPEN_BRACKET (NUMBER_BASE=NUM|VAR_BASE=ID) COMMA (NUMBER_EXP=NUM|VAR_EXP=ID) CLOSE_BRACKET;

// Methods for printing to screen (println, print)
println_statement: PRINTLN OPEN_BRACKET (MESSAGE=STRING_LIT|VAR=ID|SQUARE_ROOT=square_root|absolute_value|PI_CONST=PI | func_call) CLOSE_BRACKET;
print_statement: PRINT OPEN_BRACKET (MESSAGE=STRING_LIT|VAR=ID|SQUARE_ROOT=square_root|absolute_value|PI_CONST=PI | func_call) CLOSE_BRACKET;
