# T# v3.0.0-alpha1

The T# Programming Language — written by Dylan Armstrong, 2026, in C++ with ANTLR.

T# is a statically typed, C#/Java-inspired programming language with classes, interfaces, enums, generics, imports, a growing standard library, and a native interpreter runtime.

## Highlights

- C#/Java-style syntax
- Static type declarations
- Classes, inheritance, interfaces and enums
- Generic classes such as `Dictionary<K,V>`, `Vector2<T>`, `Vector3<T>`, `Matrix2<T>` and `Matrix3<T>`
- Multi-file programs with `import`
- Standard library modules
- Built-in math, file, array, type and system functions
- Runtime memory address inspection through `address(value)`
- ANTLR-based parser
- C++20 interpreter runtime

## Changelog

### v3.0.0

#### New Features and Fixes:
---
**Static Typing/Type Safety (MASSIVE enhancements)**: A much better static type system is now in the language. 

#### How does it work? 
T# now has a **compile-time static-type checking system**. The compiler (or really interpreter in this case), infers the expression types, resolves symbols, checks assignments and method calls for their types. Then, ONLY if there are no errors (including type errors) is the T# bytecode generated.

#### What about generic types?
When a method call of a generic class's method is called it constructs a substituion map e.g.
```
K -> string
V -> int
```
It then susbtitutes the generic parameter types in the method signature before checking the supplied arguments.

#### With spawn/await concurrency?
T# (see the next part below on concurrency/multithreading), now has `spawn` and `await` for concurrency (similiar in a way to Go's goroutines). The way this works with static typing is similiar but the type checker treats `spawn` as producing a task and `await` as extracting the return type of that task.

So:
`await task<long>` actually produces a value of type `long`.

#### Is it type safe?
Short answer: No. It definetly has a sound type system now (which is LOTS better than the old system), but I still wouldn't call it type safe. Why? Certain expressions could be out there that can produce an empty type. Type systems are one of the hardest parts of any language. Further testing is required.

---

**Concurrency and multithreading in T# (WIP)**: As mentioned in the type system part of this version changelog, added `spawn` to spawn tasks on different threads in T# and `await` to call those tasks spawned using `spawn`. This means multiple tasks can be ran on different threads allowing for T# concurrency and multithreading - giving great performance benefits.

---

**Nullable/optional types (WIP)**: nullable/optional support in T#. If you use a `?` after the type name - you can make the variable a nullable meaning it can have a value of the given type OR be marked `null`. This supports null safety.

---

**readln**: The `readln` function in T# - allows for a developer to take user's input in T#.

---

**datetime**: `datetime_now()` allows for the current date and time to be stored in a value of type `long`, whilst you can format it also using `datetime_format()` and add days to the datetime using `datetime_add_days(datetime: long, days: int)`. For an example, see: `examples/datetime.tsharp`.

---

#### New Standard Library Data Types
**std.math.Calculus**: A data type that adds support for calculus operations (such as differentiation, integration, etc. in T#)

**std.math.Optimization** A data type that adds support for optimization and gradient descent in T#.

**std.math.Statistics** A data type that adds support for operations such as mean, standard deviation, etc in T#.

---

### v2.0.0

Full Bytecode system, ready for production usage.

### v2.0.0-beta1

Bytecode update. The whole language now runs on a VM/bytecode system instead of the old tree walking interpreter, giving HUGE performance benefits.

### v1.0.0

Final bug fixes primarily in the ANTLR graz1mmar specification.

### v1.0.0-betarc

This will be the final beta before a full release of the language.

**Fixes**:

- Fixed bugs with arrays. This includes bugs with array types and array intitialisation within the interpreter. Arrays should now behave much better than what they did before.

**General Code Patching**:

- Commented out the T# codebase a lot more.
- Added formatting properly via `clang-format`.

### v1.0.0-beta5

Added a larger standard library and developer tooling scaffolding.

**New Standard Library Math Types**

- `std.math.Vector2`
- `std.math.Vector3`
- `std.math.Matrix2`
- `std.math.Matrix3`

**New Standard Library Areas**

- `std.collections.Dictionary`
- `std.io.File`
- `std.math`

**Developer Tooling**

- Added a VS Code extension scaffold.
- Added TextMate grammar highlighting for `.tsharp`.
- Added snippets for common T# declarations.
- Added an LSP server scaffold for diagnostics, completions, hover and document symbols.

**Type System Progress**

- Added stricter runtime-backed type validation for variables, fields, arguments and return values.
- Added initial support for generic placeholder types such as `T`, `K` and `V`.
- Added support for constructor calls inside expressions, e.g. `return Vector2<T>(x, y)`.

### v1.0.0-beta4

- Multi-file program support.
- New `address(value)` built-in function.
- Fixed instance method calls such as `obj.Serialize()`.
- Fixed parser/tree lifetime issues affecting multi-file execution.
- Fixed inherited field initialisation for derived classes.
- Improved object member access and assignment.
- Added function argument count validation.
- Added constructor arity validation.
- Corrected block control flow for `return`, `break` and `continue`.
- Fixed numeric `+=`, `-=` and floating-point subtraction behaviour.
- Fixed `print()` and `println()` to correctly print all supplied arguments.

### v1.0.0-beta3

- Improved object/member calls.
- Added multi-file loading foundations.
- Added runtime safety improvements.

### v1.0.0-beta2

- Added type casting methods.
- Added `size()`, `sort()`, `push()`, `pop()` and `typeof()`.

## Building

T# requires ANTLR and C++20.

### Windows

```powershell
.\build.ps1
```

### macOS / Linux

```bash
./build.sh
```

## Running Programs

Run a single file:

```powershell
.\build\Release\tsharp.exe .\examples\demo.tsharp
```

Run a program that uses imports:

```powershell
.\build\Release\tsharp.exe .\examples\std_demo.tsharp
```

## Imports

```tsharp
import std.collections.Dictionary
import std.io.File
import std.math.Vector2
import std.math.Vector3
import std.math.Matrix2
import std.math.Matrix3
```

## Standard Library Examples

### Dictionary

```tsharp
import std.collections.Dictionary

void main() {
    Dictionary<string, int> d()
    d.put("Test", 5)
    println(d.get("Test"))
    println(d.count())
}
```

### File I/O

```tsharp
import std.io.File

void main() {
    File f()
    string text = f.read_all_text("examples/test.txt")
    println(text)
}
```

### Vector2

```tsharp
import std.math.Vector2

void main() {
    Vector2<int> a(5, 10)
    Vector2<int> b(1, 13)
    println(a.add(b).to_string())
}
```

### Vector3

```tsharp
import std.math.Vector3

void main() {
    Vector3<int> a(1, 2, 3)
    Vector3<int> b(4, 5, 6)
    println(a.add(b).to_string())
    println(a.cross(b).to_string())
}
```

### Matrix2

```tsharp
import std.math.Matrix2

void main() {
    Matrix2<int> m(1, 2, 3, 4)
    println(m.to_string())
    println(m.determinant())
}
```

### Matrix3

```tsharp
import std.math.Matrix3

void main() {
    Matrix3<int> m(1, 2, 3, 0, 1, 4, 5, 6, 0)
    println(m.to_string())
    println(m.determinant())
}
```

## Built-in Constants

| Name           | Description             |
| -------------- | ----------------------- |
| `pi`           | Mathematical constant π |
| `e`            | Euler's number          |
| `tau`          | 2π                      |
| `golden_ratio` | Golden ratio            |

## Built-in Functions

| Function             | Description                                                |
| -------------------- | ---------------------------------------------------------- |
| `print(value)`       | Prints without a newline                                   |
| `println(value)`     | Prints with a newline                                      |
| `size(value)`        | Returns array length, string length or runtime value size  |
| `typeof(value)`      | Returns the runtime type name                              |
| `address(value)`     | Returns the runtime memory address as a hexadecimal string |
| `push(array, value)` | Pushes a value into an array                               |
| `pop(array)`         | Removes and returns the last array element                 |
| `sort(array)`        | Sorts an array                                             |
| `sqrt(x)`            | Square root                                                |
| `abs(x)`             | Absolute value                                             |
| `pow(x, y)`          | Power                                                      |
| `sin(x)`             | Sine in radians                                            |
| `cos(x)`             | Cosine in radians                                          |
| `tan(x)`             | Tangent in radians                                         |
| `min(a, b)`          | Minimum                                                    |
| `max(a, b)`          | Maximum                                                    |
| `factorial(n)`       | Factorial                                                  |

## Open Source and Free

T# is open source and free to use, run and contribute to.

Pull requests are welcome.

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
| `long`         | Type          | Stores bigger whole numbers than `int`                                    |
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
| `size()`       | Function      | Returns number of elements in array or string length                      |
| `typeof()`     | Function      | Returns the type of a value as a string                                   |
| `address()`    | Function      | Returns the runtime memory address of a value as a hexadecimal string     |
| `push()`       | Function      | Adds an element to the end of an array                                    |
| `pop()`        | Function      | Removes and returns the last element of an array                          |
| `sort()`       | Function      | Sorts an array in ascending order                                         |
| `print()`      | Function      | Prints to console without newline                                         |
| `println()`    | Function      | Prints to console with newline                                            |
| `.to_int()`    | Method        | Converts a value to int                                                   |
| `.to_float()`  | Method        | Converts a value to float                                                 |
| `.to_double()` | Method        | Converts a value to double                                                |
| `.to_string()` | Method        | Converts a value to string                                                |
| `.to_bool()`   | Method        | Converts a value to bool                                                  |
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
| `(` `)`        | Delimiter     | Parentheses for grouping expressions and parameters                       |
| `{` `}`        | Delimiter     | Braces for code blocks                                                    |
| `[` `]`        | Delimiter     | Brackets for array access and array type declarations                     |
| `,`            | Delimiter     | Comma for separating items                                                |
| `.`            | Delimiter     | Dot for member access                                                     |
| `:`            | Delimiter     | Colon for inheritance clauses                                             |
| `;`            | Delimiter     | Semicolon for loop separators                                             |

## Full Documentation

Documentation: https://tsharp.dylanarmstrong.net

## Credits

Written and founded by Dylan Armstrong in 2026.

Have fun.
