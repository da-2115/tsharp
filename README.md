# tsharp v1.0.0-alpha3
The T# Programming Language - Written by Dylan Armstrong, 2026 in C++ with the ANTLR framework.

# How to Run?

**You must have ANTLR installed and T# uses the C++20 standard (CMake should automatically set the standard for you).**

1. `cmake .`
2. `make`
3. `./tsharp ./your_file_name_here.tsharp` OR `tsharp --version`

There are three current examples in the `examples/` folder.

# Open Source and Free
T# is an open source and free language, now accepting pull requests (PRs). The language is also free - there is no cost to use, run and contribute to T#.

| Token          | Type of Token | Description                                                               |
| -------------- | ------------- | ------------------------------------------------------------------------- |
| `int`          | Type          | Stores whole numbers (positive and negative)                              |
| `float`        | Type          | Stores floating point (decimal) numbers                                   |
| `double`       | Type          | Floating point with double precision                                      |
| `string`       | Type          | A line of text                                                            |
| `char`         | Type          | A single character                                                        |
| `bool`         | Type          | True or false                                                             |
| `void`         | Type          | Does not return anything                                                  |
| `any`          | Type          | Can be any type                                                           |
| `class`        | Keyword       | Declares a class for object-oriented programming                          |
| `interface`    | Keyword       | Declares an interface defining a contract for classes                     |
| `enum`         | Keyword       | Declares an enumeration of named constants                                |
| `abstract`     | Modifier      | Makes a class or method abstract (cannot be instantiated/called directly) |
| `static`       | Modifier      | Makes a member belong to the class rather than instances                  |
| `public`       | Modifier      | Makes a member accessible from anywhere                                   |
| `private`      | Modifier      | Makes a member accessible only within the same class                      |
| `protected`    | Modifier      | Makes a member accessible within the class and derived classes            |
| `virtual`      | Modifier      | Allows a method to be overridden in derived classes                       |
| `override`     | Modifier      | Overrides a virtual method from a base class                              |
| `if`           | Keyword       | Conditional statement for branching logic                                 |
| `else`         | Keyword       | Alternative branch for if statements                                      |
| `for`          | Keyword       | Loop statement with initialization, condition, and update                 |
| `while`        | Keyword       | Loop statement that continues while a condition is true                   |
| `do`           | Keyword       | Loop statement that runs at least once before checking condition          |
| `switch`       | Keyword       | Multiple choice statement based on expression value                       |
| `case`         | Keyword       | Label for a specific value in a switch statement                          |
| `default`      | Keyword       | Default case in a switch statement                                        |
| `break`        | Keyword       | Exits from a loop or switch statement                                     |
| `continue`     | Keyword       | Skips to the next iteration of a loop                                     |
| `return`       | Keyword       | Returns a value from a function or method                                 |
| `try`          | Keyword       | Begins a block for exception handling                                     |
| `catch`        | Keyword       | Catches and handles a specific exception type                             |
| `throw`        | Keyword       | Throws an exception                                                       |
| `finally`      | Keyword       | Block that always executes after try/catch                                |
| `this`         | Keyword       | Reference to the current object instance                                  |
| `base`         | Keyword       | Reference to the base class                                               |
| `true`         | Literal       | Boolean true value                                                        |
| `false`        | Literal       | Boolean false value                                                       |
| `null`         | Literal       | Null reference value                                                      |
| `pi`           | Constant      | Mathematical constant π (3.14159...)                                      |
| `e`            | Constant      | Mathematical constant e (2.71828...)                                      |
| `tau`          | Constant      | Tau constant (2π)                                                         |
| `golden_ratio` | Constant      | Golden ratio constant (1.618...)                                          |
| `abs()`        | Function      | Returns absolute value                                                    |
| `sqrt()`       | Function      | Returns square root                                                       |
| `cube_root()`  | Function      | Returns cube root                                                         |
| `exp()`        | Function      | Returns exponential function (e^x)                                        |
| `log()`        | Function      | Returns logarithm (base 10)                                               |
| `sin()`        | Function      | Returns sine (radians)                                                    |
| `cos()`        | Function      | Returns cosine (radians)                                                  |
| `tan()`        | Function      | Returns tangent (radians)                                                 |
| `floor()`      | Function      | Rounds down to nearest integer                                            |
| `ceil()`       | Function      | Rounds up to nearest integer                                              |
| `round()`      | Function      | Rounds to nearest integer                                                 |
| `pow()`        | Function      | Returns power function (x^y)                                              |
| `x_root()`     | Function      | Returns nth root of x                                                     |
| `min()`        | Function      | Returns minimum of two values                                             |
| `max()`        | Function      | Returns maximum of two values                                             |
| `factorial()`  | Function      | Returns factorial of n (n!)                                               |
| `print()`      | Function      | Prints to console without newline                                         |
| `println()`    | Function      | Prints to console with newline                                            |
| `->`           | Operator      | Arrow operator for property expressions                                   |
| `=`            | Operator      | Assignment operator                                                       |
| `+=`           | Operator      | Add and assign                                                            |
| `-=`           | Operator      | Subtract and assign                                                       |
| `*=`           | Operator      | Multiply and assign                                                       |
| `/=`           | Operator      | Divide and assign                                                         |
| `%=`           | Operator      | Modulo and assign                                                         |
| `+`            | Operator      | Addition                                                                  |
| `-`            | Operator      | Subtraction                                                               |
| `*`            | Operator      | Multiplication                                                            |
| `/`            | Operator      | Division                                                                  |
| `%`            | Operator      | Modulo (remainder)                                                        |
| `==`           | Operator      | Equality comparison                                                       |
| `!=`           | Operator      | Inequality comparison                                                     |
| `<`            | Operator      | Less than comparison                                                      |
| `>`            | Operator      | Greater than comparison                                                   |
| `<=`           | Operator      | Less than or equal comparison                                             |
| `>=`           | Operator      | Greater than or equal comparison                                          |
| `&&`           | Operator      | Logical AND                                                               |
| `\|`           | Operator      | Logical OR                                                                |
| `!`            | Operator      | Logical NOT                                                               |
| `++`           | Operator      | Increment by one                                                          |
| `--`           | Operator      | Decrement by one                                                          |
| `(` `)`        | Delimiter     | Parentheses for grouping expressions and parameters                       |
| `{` `}`        | Delimiter     | Braces for code blocks                                                    |
| `[` `]`        | Delimiter     | Brackets for array access and array type declarations                     |
| `,`            | Delimiter     | Comma for separating items                                                |
| `.`            | Delimiter     | Dot for member access                                                     |
| `:`            | Delimiter     | Colon for inheritance clauses                                             |
| `;`            | Delimiter     | Semicolon for loop separators                                             |

# Full Documentation
All documentation for T# is on: https://tsharp.dylanarmstrong.net

# Credits
Written and Founded by Dylan Armstrong in 2026.

Have fun.

