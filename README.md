
# Simple Computable Ordered Notation (SCON)

<br>
<div align="center">
    <a href="https://github.com/KaiNorberg/SCON/issues">
      <img src="https://img.shields.io/github/issues/KaiNorberg/SCON">
    </a>
    <a href="https://github.com/KaiNorberg/SCON/network">
      <img src="https://img.shields.io/github/forks/KaiNorberg/SCON">
    </a>
    <a href="https://github.com/KaiNorberg/SCON/stargazers">
      <img src="https://img.shields.io/github/stars/KaiNorberg/SCON">
    </a>
    <a href="https://github.com/KaiNorberg/SCON/blob/main/license">
      <img src="https://img.shields.io/github/license/KaiNorberg/SCON">
    </a>
    <br>
</div>
<br>

The SCON language provides a flexible, simple, efficient and Turing complete way to store and manipulate hierarchical data, all within a single freestanding C99 header.

> SCON is currently very work in progress.

## Setup and Usage

Included is an example of using SCON as a single header without linking:

```c
// my_file.c

#define SCON_INLINE
#include "scon.h"
#include "scon_libc.h" // Provides scon_libc_callbacks() for using SCON with libc

#include <stdio.h>
#include <stdlib.h>

char buffer[0x10000];

int main(int argc, char **argv)
{    
    scon_callbacks_t callbacks = scon_libc_callbacks();
    scon_t* scon = scon_new(&callbacks);
    if (scon == NULL) 
    {
        return 1;
    }
    
    int result = 0;
    if (!scon_parse_file(scon, "my_file.scon"))
    {
        printf(scon_error_get(scon));
        result = 1;
        goto cleanup;
    }

    if (!scon_eval(scon))
    {
        printf(scon_error_get(scon));
        result = 1;
        goto cleanup;
    }

    scon_stringify(scon, buffer, sizeof(buffer));
    printf("%s\n", buffer);

cleanup:
    scon_free(scon);

    return result;
}
```

Included is another example of using SCON with linking where an additional implementation file is used to build the SCON parser and evaluator:

```c
// my_file.c

#include "scon.h"
#include "scon_libc.h"

#include <stdio.h>
#include <stdlib.h>

char buffer[0x10000];

int main(int argc, char **argv)
{
    /// Same as above...
}
```

```c
// my_scon.c

#define SCON_IMPL
#include "scon.h"
```

For examples on how to write SCON, see the `bench/` and `tests/` directories.

## Benchmarks

Included below are the results of example benchmarks comparing SCON with python 3.14.3 and Lua 5.4.8 using hyperfine, all benchmarks performed in Fedora 43 (6.19.11-200.fc43.x86_64).

### Fib20

Benchmark to find the 20th Fibonacci number.

| Command | Mean [ms] | Min [ms] | Max [ms] | Relative |
|:---|---:|---:|---:|---:|
| `python bench/fib20.py` | 15.0 ± 1.3 | 13.1 | 24.6 | 9.02 ± 2.35 |
| `scon bench/fib20.scon` | 5.7 ± 0.6 | 4.9 | 8.8 | 3.44 ± 0.92 |
| `lua bench/fib20.lua` | 1.7 ± 0.4 | 1.2 | 4.4 | 1.00 |

### Fib30

Benchmark to find the 30th Fibonacci number.

| Command | Mean [ms] | Min [ms] | Max [ms] | Relative |
|:---|---:|---:|---:|---:|
| `python bench/fib30.py` | 119.2 ± 6.5 | 108.7 | 135.1 | 1.59 ± 0.13 |
| `scon bench/fib30.scon` | 545.3 ± 10.1 | 531.2 | 566.9 | 7.28 ± 0.45 |
| `lua bench/fib30.lua` | 74.9 ± 4.4 | 70.5 | 92.8 | 1.00 |

## Tools

### Command Line Interface (CLI)

A simple CLI tool is provided to evaluate SCON files or expressions directly from the terminal.

```bash
scon my_file.scon
scon -e "(+ 1 2 3)"
scon -s my_file.scon # output the final heirarchical data
```

#### Setup

The CLI tool can be found at `tools/scon/` and uses CMake. To build the tool write:

```bash
cd tools/scon
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

To install the tool write:

```bash
cmake --install build # if that fails: sudo cmake --install build
```

### Visual Studio Code

A syntax highlighting extension for Visual Studio Code can be found at `tools/scon-vscode/`.

#### Setup

The extension is not yet available in the marketplace.

As such, to install it, copy the `tools/scon-vscode/` directory to `%USERPROFILE%\.vscode\extensions\` if your on Windows or `~/.vscode/extensions/` if your on macOS/Linux.

Finally, restart Visual Studio Code.

## Grammar

The grammar of SCON is designed to be as straight forward as possible, the full grammar using [EBNF](https://en.wikipedia.org/wiki/Extended_Backus%E2%80%93Naur_form) can be found below.

```ebnf
file = { expression | comment } ;
expression = list | atom | white_space ;
item = list | atom ;

list = "(", { expression }, ")" ;

atom = unquoted_atom | quoted_atom ;
unquoted_atom = ( character, { character } ) ;
quoted_atom = '"', { character | white_space | escape_sequence }, '"' ;

comment = line_comment | block_comment ;
line_comment = "//", { character | " " | "\t" }, [ "\n" | "\r" ] ;
block_comment = "/*", { character | white_space }, "*/" ;

white_space = " " | "\t" | "\n" | "\r" ;
escape_sequence = "\\", ( "a" | "b" | "e" | "f" | "n" | "r" | "t" | "v" | "\\" | "'" | '"' | "?" | ( "x", hex, hex ) ) ;

character = letter | digit | symbol ;

symbol = sign | "!" | "$" | "%" | "&" | "*" | "." | "/" | ":" | "<" | "=" | ">" | "?" | "@" | "^" | "_" | "|" | "~" | "{" | "}" | "[" | "]" | "#" ;
sign = "+" | "-" ;

hex = digit | "A" | "B" | "C" | "D" | "E" | "F" | "a" | "b" | "c" | "d" | "e" | "f" ;
digit = octal | "8" | "9" ;
octal = binary | "2" | "3" | "4" | "5" | "6" | "7" ;
binary = "0" | "1" ;

letter = upper_case | lower_case ;
upper_case = "A" | "B" | "C" | "D" | "E" | "F" | "G" | "H" | "I" | "J" | "K" | "L" | "M" | "N"| "O" | "P" | "Q" | "R" | "S" | "T" | "U"| "V" | "W" | "X" | "Y" | "Z" ;
lower_case = "a" | "b" | "c" | "d" | "e" | "f" | "g" | "h" | "i" | "j" | "k" | "l" | "m" | "n"| "o" | "p" | "q" | "r" | "s" | "t" | "u"| "v" | "w" | "x" | "y" | "z" ;
```

> Note the difference between the terms "item" and "expression", the term "item" will be used to refer to the in memory structure while a "expression" refers to the textual representation of an item. Meaning that an expression evaluates to an item.

Included is a list of the final value for each escape sequence:

<div align="center">
| Escape Sequence | Description | Final Value |
|-----------------|-------------|-------------|
| `\a` | Alert (bell) | `0x07` |
| `\b` | Backspace | `0x08` |
| `\e` | Escape | `0x1B` |
| `\f` | Formfeed | `0x0C` |
| `\n` | Newline | `0x0A` |
| `\r` | Carriage Return | `0x0D` |
| `\t` | Horizontal Tab | `0x09` |
| `\v` | Vertical Tab | `0x0B` |
| `\\` | Backslash | `\` |
| `\'` | Single Quote | `'` |
| `\"` | Double Quote | `"` |
| `\?` | Question Mark | `?` |
| `\xHH` | Hexadecimal value | `0xHH` |
</div>

## Evaluation

A parsed SCON expression can optionally be evaluated. This means that everything described below is an optional extension
of the language, not a core part of it, and should be considered as such. This can be quite convenient as it allows SCON to be utilized as a simple and efficient markup language similar to JSON or XML.

Evaluation is the process of recursively reducing an expression to its simplest form in a depth-first, left-to-right
manner.

### Lambdas and Primitives

There are two callable types, lambdas and primitives. Lambdas as defined as expressions within SCON, while primitives are built-in functions provided by the evaluator. It is possible to define additional primitives within C.

```ebnf
callable = primitive | lambda ;
primitive = "(", unquoted_atom, { expression }, ")" ;
lambda = "(", "lambda", { expression }, ")" ;
```

### Atoms

There are no integers, floats or similar within SCON, only atoms with different "shapes".

An atom can be either string-shaped, integer-shaped or float-shaped, for convenience an atom that is integer-shaped or float-shaped is also considered number-shaped.

```ebnf
string = quoted_atom ;

number = integer | float ;
integer = decimal_integer | hex_integer | octal_integer | binary_integer ;

decimal_integer = [ sign ], digit, { digit | "_" } ;
hex_integer = [ sign ], "0", ( "x" | "X" ), ( hex_digit ), { hex_digit | "_" } ;
octal_integer = [ sign ], "0", ( "o" | "O" ), ( octal_digit ), { octal_digit | "_" } ;
binary_integer = [ sign ], "0", ( "b" | "B" ), ( binary_digit ), { binary_digit | "_" } ;

float = float_number | float_naked_decimal | scientific_number | special_float ;

float_number = [ sign ], digit, { digit | "_" }, ".", digit, { digit | "_" }, [ ( "e" | "E" ), [ sign ], digit, { digit | "_" } ] ;
float_naked_decimal = [ sign ], ".", digit, { digit | "_" }, [ ( "e" | "E" ), [ sign ], digit, { digit | "_" } ] ;
scientific_number = [ sign ], digit, { digit | "_" }, ( "e" | "E" ), [ sign ], digit, { digit | "_" } ;
special_float = ( [ sign ], ( "i" | "I" ), ( "n" | "N" ), ( "f" | "F" ) ) | ( ( "n" | "N" ), ( "a" | "A" ), ( "n" | "N" ) ) ;
```

All mathematical operations on integer-shaped or float-shaped atoms follow C semantics.

#### Type Coercion

For any math primitive that takes in multiple arguments, C-like type promotion rules are used.

This means that if any of the atoms provided to the operation are float-shaped, the others will also be converted to floats. Otherwise, they will be converted to integers.

If one or more of the atoms are neither integer-shaped nor float-shaped, the evaluation will fail.

### Lists

During the evaluation of a list, the first item is immediately evaluated and if it evaluates to a callable item (a primitive or a lambda), it will be executed with the remaining items in the list as its arguments and with the list being replaced by the result of the evaluation.

Certain primitives may not evaluate all items within the list, for example, the `or` primitive will stop evaluating on the first truthy item. Most primitives, such as `+`, will evaluate all items.

### Truthiness

The following grammar defines truthy and falsy items:

```ebnf
truthy = true | expression - falsy_item ;
falsy  = falsy_list | falsy_atom | zero ;

falsy_list = nil ;
falsy_atom = '""' | false ;

true = "true" ;
false = "false" ;

nil = "(", { white_space }, ")" ;

zero = decimal_zero | hex_zero | octal_zero | binary_zero ;
decimal_zero = [ sign ], "0", { "0" | "_" }, [ ".", { "0" | "_" } ], [ ( "e" | "E" ), [ sign ], digit, { digit | "_" } ] ;
hex_zero     = [ sign ], "0", ( "x" | "X" ), "0", { "0" | "_" } ;
octal_zero   = [ sign ], "0", ( "o" | "O" ), "0", { "0" | "_" } ;
binary_zero  = [ sign ], "0", ( "b" | "B" ), "0", { "0" | "_" } ;
```

### Ordering

SCON defines a total ordering for all possible items. This is used by comparison primitives like `<` or `sort`.

The ordering of types is defined as

```plaintext
number < string < list
```

#### Ordering Rules

- **number:** Compared by value, a greater value is considered larger. If both values are equal, but one is float-shaped and the other is integer-shaped, the float-shaped atom is considered larger.
- **string:** Compared lexicographically by their ASCII characters (e.g., `"apple" < "banana"`), or by their length if one is a prefix of the other (e.g., `"apple" < "apples"`).
- **list:** Compared item by item. A list is considered "less than" another if its first non-equal item is lesser than the other list's first non-equal item (e.g., `(1 2) < (1 3)`), or by their length if one is a prefix of the other (e.g., `(1 2) < (1 2 3)`).

### Variables

Variables are used to store and retrieve items within a SCON environment. Variables are defined using the `def`
primitive and can be accessed using their names.

As an example, variables can be used to create a more traditional "function definition" by defining a variable as a lambda:

```lisp
(def add (lambda (a b) (+ a b)))

(add 1 2) // Evaluates to "3"
```

### Lexical Scoping

SCON uses lexical scoping. This means that a function or expression can access variables from the scope in which it was defined, as well as any parent scopes. When a variable is defined using `def`, it is added to the current local scope. If a variable is accessed, the evaluator searches for the name starting from the innermost scope and moving outwards to the global scope.

When a lambda is executed, it creates a new scope where its arguments are bound to the provided values. This scope is destroyed once the lambda finishes evaluation, ensuring that local variables do not leak into the outer environment.

### Default Constants

The following constants are defined by default in the SCON environment:

<div align="center">
| Constant | Value |
|----------|-------|
| `true`   | `true` |
| `false`  | `false` |
| `nil`    | `()` |
| `pi`     | `3.14159265358979323846` |
| `e`      | `2.7182818284590452354` |
</div>

### Default Primitives

The primitives below are specified using a format similar to the EBNF format, taking advantage of previously made definitions.

#### Core & Evaluation

**`(quote <expression>) -> <expression>`**

Returns the provided expression without evaluating it.

**`(list {expression} ) -> <list>`**

Returns a new list containing the results of evaluating each expression.

**`(do <expression> {expression}) -> <item>`**

Evaluates each expression in sequence and returns the result of the last one.

---

#### Variables & Scope

**`(def <name: atom> <value: item>) -> <value: item>`**
  
Defines a variable with the given name and value within the current scope.

**`(set <name: atom> <value: item>) -> <value: item>`**

Modifies the value of an existing variable with the given name in the closest scope where it is defined.

**`(let ( { ( <name: atom> <value: expression>) } ) <body: expression> {body: expression} ) -> <item>`**

Evaluates all expressions within a new local scope with the specified variables defined, returning the result of the last body.

Each variable is evaluated sequentially from left-to-right, meaning a variable to the right can reference a variable to the left.

---

#### Control Flow & Logic

**`(if <cond: item> <then: item> [else: item]) -> <item>`**

Evaluates `<then>` if `<cond>` is truthy, otherwise evaluates `<else>` if provided, otherwise returns `nil`.

**`(when <cond: item> {body: item}) -> <item>`**

Evaluates each `<body>` expression in sequence if `<cond>` is truthy, returning the result of the last expression, or `false` if the condition is falsy.

**`(unless <cond: item> {body: item}) -> <item>`**

Evaluates each `<body>` expression in sequence if `<cond>` is falsy, returning the result of the last expression, or `nil` if the condition is truthy.

**`(cond ( <cond: item> <val: item> ) { ( <cond: item> <val: item> ) }) -> <item>`**

Evaluates each condition in order, returning the value associated with the first truthy condition, or `nil` if none are truthy.

**`(and <item> {item}) -> <item>`**

Evaluates arguments left-to-right, returning the last truthy value, or `false` if any are falsy. Will stop evaluating arguments as soon as one is falsy.

**`(or <item> {item}) -> <item>`**

Evaluates arguments left-to-right, returning the first truthy value, or `false` if all are falsy. Will stop evaluating arguments as soon as one is truthy.

**`(not <item>) -> <true|false>`**

Returns `true` if the argument is falsy, otherwise `false`.

---

#### Error Handling

**`(assert <cond: item> <msg: item>)`**

Evaluates `<cond>`. If it is falsy, the evaluation fails and throws an error with `<msg>` as the message.

**`(throw <msg: item>)`**

Throws an error with the given atom being the error message.

**`(catch <expression> <handler: lambda>) -> <item>`**

Evaluates `<expression>`. If an error occurs during evaluation, the `<handler>` lambda is called with the error message as its argument, and its result is returned. If no error occurs, the result of `<expression>` is returned.

---

#### Functions & Higher-Order

**`(lambda ( {arg: atom} ) <expression> {expression} ) -> <lambda>`**

Returns a user-defined anonymous function. When called, the body expressions are evaluated in sequence, and the result of the last expression is returned.

**`(map <callable> <list>) -> <list>`**

Returns a new list by applying `<callable>` to each item in `<list>`. The `<callable>` must accept a single argument.

**`(filter <callable> <list>) -> <list>`**

Returns a new list containing only items from `<list>` for which `<callable>` returns a truthy value. The `<callable>` must accept a single argument.

**`(reduce <callable> <initial> <list>) -> <item>`**

Reduces `<list>` to a single value. The `<callable>` must accept two arguments: the `accumulator` (which starts as `<initial>`) and the current `item`, and it should return the new accumulator value.

**`(sort <list> [callable]) -> <list>`**

Returns a new list containing the sorted items of the list. The sort is guaranteed to be stable and to minimize the number of comparisons made.

If a `callable` is provided, it is used as a custom comparator that should accept two arguments and return a truthy value if the first argument should appear before the second.

If no `callable` is specified, then items are sorted in ascending order.

---

#### Arithmetic

**`(+ {item})`**

Returns the sum of all arguments. If only one argument is provided, returns its self.

**`(- {item})`**

Returns the result of subtracting the sum of all arguments from the first argument. If only one argument is provided, returns its negation.

**`(* {item})`**

Returns the product of all arguments. If only one argument is provided, returns its self.

**`(/ {item})`**

Returns the result of dividing the first argument by the subsequent arguments. If only one argument is provided, returns its reciprocal.

If a division by zero occurs, the evaluation fails.

**`(% <item> <item>)`**

Returns the remainder of dividing the first argument by the second (modulo).

If a division by zero occurs, the evaluation fails.

---

#### Comparison

**`(= <item> <item> {item} )`**

Returns `true` if all arguments are equal (numerically if all are numbers, otherwise by string comparison), otherwise `false`.

Will stop evaluating arguments as soon as one is not equal.

**`(== <item> <item> {item} )`**

Returns `true` if all arguments are exactly equal using string comparison, otherwise `false`.

Will stop evaluating arguments as soon as one is not equal.

**`(!= <item> <item> {item} )`**

Returns `true` if any two adjacent arguments are not equal, otherwise `false`.

Will stop evaluating arguments as soon as one is equal.

**`(< <item> <item> {item})`**

Returns `true` if each argument is less than the next, otherwise `false`.

Will stop evaluating arguments as soon as one is not less than the next.

**`(<= <item> <item> {item})`**

Returns `true` if each argument is less than or equal to the next, otherwise `false`.

Will stop evaluating arguments as soon as one is not less than or equal to the next.

**`(> <item> <item> {item})`**

Returns `true` if each argument is greater than the next, otherwise `false`.

Will stop evaluating arguments as soon as one is not greater than the next.

**`(>= <item> <item> {item})`**

Returns `true` if each argument is greater than or equal to the next, otherwise `false`.

Will stop evaluating arguments as soon as one is not greater than or equal to the next.

---

> Primitives below this line will need to have their documentation updated.

#### Bitwise

##### `(& <val1> <val2> ...)`

Returns the bitwise AND of all arguments.

##### `(| <val1> <val2> ...)`

Returns the bitwise OR of all arguments.

##### `(^ <val1> <val2> ...)`

Returns the bitwise XOR of all arguments.

##### `(~ <val>)`

Returns the bitwise NOT of the argument.

##### `(<< <val> <shift>)`

Returns the value bitwise shifted left.

##### `(>> <val> <shift>)`

Returns the value bitwise shifted right.

---

#### Math & Trigonometry

##### `(min <val1> <val2> ...)`

Returns the smallest of all arguments.

##### `(max <val1> <val2> ...)`

Returns the largest of all arguments.

##### `(clamp <val> <min> <max>)`

Restricts a value to be between the given minimum and maximum.

##### `(abs <val>)`

Returns the absolute value of the argument.

##### `(floor <val>)`

Returns the largest integer less than or equal to the argument.

##### `(ceil <val>)`

Returns the smallest integer greater than or equal to the argument.

##### `(round <val>)`

Returns the argument rounded to the nearest integer.

##### `(pow <base> <exp>)`

Returns the base raised to the power of the exponent.

##### `(log <val> [base])`

Returns the natural logarithm of the argument, or the specified logarithm of the argument.

##### `(sqrt <val>)`

Returns the square root of the argument.

##### `(sin <val>)`

Returns the sine of the argument.

##### `(cos <val>)`

Returns the cosine of the argument.

##### `(tan <val>)`

Returns the tangent of the argument.

##### `(asin <val>)`

Returns the arcsine of the argument.

##### `(acos <val>)`

Returns the arccosine of the argument.

##### `(atan <val>)`

Returns the arctangent of the argument.

##### `(atan2 <y> <x>)`

Returns the arctangent of the quotient of its arguments.

##### `(sinh <val>)`

Returns the hyperbolic sine of the argument.

##### `(cosh <val>)`

Returns the hyperbolic cosine of the argument.

##### `(tanh <val>)`

Returns the hyperbolic tangent of the argument.

##### `(asinh <val>)`

Returns the inverse hyperbolic sine of the argument.

##### `(acosh <val>)`

Returns the inverse hyperbolic cosine of the argument.

##### `(atanh <val>)`

Returns the inverse hyperbolic tangent of the argument.

##### `(rand <min> <max>)`

Returns a random number between the given range.

##### `(seed <val>)`

Seeds the random number generator.

---

#### System & Environment

##### `(include <path>)`

Returns the result of evaluating the SCON file at the given path, variables defined in the included file will be available in the current scope.

##### `(read-file <path>)`

Reads the file at the given path and returns its contents as a raw string atom without evaluating it.

**`(print { item } )`**

Prints the string representation of all arguments to the standard output.

**(println { item } )**

Prints the string representation of all arguments to the standard output, followed by a newline.

##### `(format <format> <item1> <item2> ...)`

Returns a formatted string using python-like formatting, where `{}` is used as a placeholder for the provided arguments.
Positional arguments can be used to specify the index of the argument to be used, for example `{0}`.

```lisp
(format "Hello, {}!" "World") // Returns "Hello, World!"
(format "{1} {0}" "World" "Hello") // Returns "Hello World"
```

##### `(time)`

Returns the current time in seconds since the Unix epoch.

##### `(env <name>)`

Returns the value of the environment variable as an atom, or an empty string if it is not set.

---

#### Type Checking & Introspection

##### `(len <item>)`

Returns the number of items in a list or the number of characters in an atom.

##### `(atom? <item>)`

Returns `true` if the item is an atom, otherwise `false`.

##### `(int? <item>)`

Returns `true` if the item is an integer shaped atom, otherwise `false`.

##### `(float? <item>)`

Returns `true` if the item is a float shaped atom, otherwise `false`.

##### `(number? <item>)`

Returns `true` if the item is an integer or float shaped atom, otherwise `false`.

##### `(lambda? <item>)`

Returns `true` if the item is a lambda, otherwise `false`.

##### `(primitive? <item>)`

Returns `true` if the item is a primitive, otherwise `false`.

##### `(callable? <item>)`

Returns `true` if the item is a lambda or primitive, otherwise `false`.

##### `(list? <item>)`

Returns `true` if the item is a list, otherwise `false`.

##### `(empty? <item>)`

Returns `true` if the item is an empty list `()`, or an empty atom `""`, otherwise `false`.

---

#### Type Casting

##### `(int <atom>)`

Returns the integer shaped representation of the atom.

##### `(float <atom>)`

Returns the float shaped representation of the atom.

---

#### Sequences (Lists & Strings)

##### `(list <item1> <item2> ...)`

Returns a new list with the provided items.

##### `(concat <item1> <item2> ...)`

Returns a new atom or list by concatenating all items, can also be utilized for "append" or "prepend" operations. If any of the items is a list, the result will be a list, otherwise it will be an atom.

##### `(first <item>)`

Returns the first item of a list or the first character of an atom as a new atom.

##### `(last <item>)`

Returns the last item of a list or the last character of an atom as a new atom.

##### `(rest <item>)`

Returns a new list containing all except the first item of a list or an atom containing all except the first character of an atom.

##### `(init <item>)`

Returns a new list containing all but the last item of a list or an atom containing all but the last character of an atom.

##### `(nth <item> <n>)`

Returns the n-th item of a list or the n-th character of an atom as a new atom, if n is negative, it returns the n-th item from the end.

##### `(index <item> <subitem>)`

Returns the index of the first occurrence of the subitem in the item.

##### `(reverse <item>)`

Returns a new list containing the items of `<item>` in reverse order or an atom containing the characters of `<item>` in reverse order.

##### `(slice <item> <start> <end>)`

Returns a sub-list or sub-atom of `<item>` starting from the `<start>` index to the `<end>` index.

---

#### String Manipulation

##### `(starts-with? <item> <prefix>)`

Returns `true` if the provided item is an atom that starts with the prefix or a list whose first item starts with the prefix, otherwise `false`.

##### `(ends-with? <item> <suffix>)`

Returns `true` if the provided item is an atom that ends with the suffix or a list whose first item ends with the suffix, otherwise `false`.

##### `(contains? <item> <subitem>)`

Returns `true` if the provided item is an atom that contains the subitem as a substring or a list that contains an item equal to the subitem, otherwise `false`.

##### `(replace <item> <old> <new>)`

Returns a new atom or list with all occurrences of `<old>` replaced by `<new>`.

##### `(join <list> <separator>)`

Returns a new atom created by joining all items in `<list>` with `<separator>`.

##### `(split <atom> <separator>)`

Returns a new list by splitting `<atom>` into sub-atoms at each occurrence of `<separator>`.

##### `(upper <atom>)`

Returns a new atom with all characters converted to uppercase.

##### `(lower <atom>)`

Returns a new atom with all characters converted to lowercase.

##### `(trim <atom>)`

Returns a new atom with leading and trailing whitespace removed.

---

#### Association Lists (Dictionaries)

##### `(get <list> <name>)`

Returns the second item of the first sub-list whose first item evaluates to `<name>`.

##### `(keys <list>)`

Returns a new list containing the first item of every sub-list.

##### `(values <list>)`

Returns a new list containing all but the first item of every sub-list.

##### `(assoc <list> <key> <value>)`

Returns a new list with the sub-list whose first item is `<key>` having the second item replaced or added with `<value>`.

##### `(dissoc <list> <key>)`

Returns a new list with the sub-list whose first item is `<key>` removed.

##### `(update <list> <key> <lambda>)`

Returns a new list with the sub-list whose first item is `<key>` having its second item updated by applying `<lambda>` to it.
