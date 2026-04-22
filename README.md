
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

SCON is a functional, immutable, and S-expression based configuration and scripting language. It aims to provide a flexible, simple, efficient and Turing complete way to store and manipulate hierarchical data, all as a freestanding C99 header-only library.

> SCON is currently very work in progress.

## Setup and Usage

Included is an example of using SCON as a single header without linking:

```c
// my_file.c

#define SCON_INLINE
#include "scon.h"

#include <stdio.h>
#include <stdlib.h>

char buffer[0x10000];

int main(int argc, char **argv)
{    
    scon_t* scon = NULL;

    scon_error_t error = SCON_ERROR();
    if (SCON_ERROR_CATCH(&error))
    {
        scon_error_print(&error, stderr);
        scon_free(scon);
        return 1;
    }

    scon = scon_new(&error);

    scon_handle_t ast = scon_parse_file(scon, "my_file.scon");

    scon_stdlib_register(scon, SCON_STDLIB_ALL);

    scon_function_t* function = scon_compile(scon, &ast);

    scon_handle_t result = scon_eval(scon, function);
    scon_stringify(scon, &result, buffer, sizeof(buffer));
    printf("%s\n", buffer);

    scon_free(scon);
    return 0;
}
```

Included is another example of using SCON with linking where an additional implementation file is used to build the SCON parser and evaluator:

```c
// my_file.c

#include "scon.h"

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

## First Steps

SCON should be familiar to anyone used to Lisp or functional languages, but it is intended to be easy to learn even for those who aren't.

All expressions in SCON are either atoms or lists of atoms, and the process of evaluating expressions ought to be thought of as reducing these expressions into a simpler form. Much like how mathematical expressions are reduced to their final values.

For example, the following SCON expression

```lisp
(+ 1 2)
(* 3 4)
```

will evaluate to `(3 12)`. Note how the result of the evaluation is a list containing the results of each top-level expression.

### Hello World

An obvious way to write "Hello World" in SCON might look like:

```lisp
(println! "Hello, World!")
```

However, it could be even simpler. We could simply write:

```lisp
"Hello, World!"
```

Which will, of course, evaluate to `Hello, World!`.

### Complex Example

Finally, included is a slightly more complex example:

```lisp
(do
    (def fib (lambda (n) 
        (if (<= n 1)
            n
            (+ (fib (- n 1)) (fib (- n 2))))
        )
    )

    (format "Fibonacci of 35 is {}" (fib 35))
)
```

## Tools

### Command Line Interface (CLI)

A simple CLI tool is provided to evaluate SCON files or expressions directly from the terminal.

```bash
scon my_file.scon
scon -e "(+ 1 2 3)"
scon -d my_file.scon # output the compiled bytecode
scon -s my_file.scon # silent mode, wont output the result of the evaluation
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

As such, to install it, copy the `tools/scon-vscode/` directory to `%USERPROFILE%\.vscode\extensions\` if you're on Windows or `~/.vscode/extensions/` if you're on macOS/Linux.

Finally, restart Visual Studio Code.

## Implementation

SCON is implemented as a register-based bytecode compiled language, where the SCON source is first parsed into an Abstract Syntax Tree (AST) and then compiled into a custom bytecode format before being executed by the virtual machine/evaluator.

> Note that the "Abstract Syntax Tree" is just a SCON expression, lists and atoms, meaning that the compiler is itself written to operate on the same data structures as  evaluator produces.
the

The bytecode format itself is a stream of 32bit instructions, with all instructions able to read/write to an array of registers, or read from an array of constants.

*See [inst.h](https://github.com/KaiNorberg/SCON/blob/main/scon/inst.h) for more information on instructions.*

Since SCON is immutable, the constants array is also used for "captured" values from outer scopes (closures) and we can also allow the compiler to fold constant expressions at compile-time, far more than would normally be possible.

*See [compile.h](https://github.com/KaiNorberg/SCON/blob/main/scon/compile.h) for more information on the compiler.*

To improve caching and reduce pointer indirection, SCON uses "handles" (`scon_handle_t`) which are [Tagged Pointers](https://en.wikipedia.org/wiki/Tagged_pointer) using NaN boxing to allow a single 64bit value to store either a 48 bit signed integer, IEEE 754 double or a pointer to a heap allocated item.

*See [handle.h](https://github.com/KaiNorberg/SCON/blob/main/scon/handle.h) for more information on handles.*

Items (`scon_item_t`) represent all heap allocated objects, such as lists, atoms and closures. All items are exactly 64 bytes in size and allocated using a custom pool allocator and freed using a garbage collector and free list.

Since SCON uses its handles to store most integers and floats, it can avoid heap allocations for many common values, significantly reducing the pressure on the garbage collector and improving caching.

*See [item.h](https://github.com/KaiNorberg/SCON/blob/main/scon/item.h) for more information on items.*

All atoms use [String Interning](https://en.wikipedia.org/wiki/String_interning), meaning that every unique atom is only stored once in memory. This makes any string comparison into a single pointer comparison, and it means that parsing the integer/floating point value of an atom or an items truthiness only needs to be done once.

*See [atom.h](https://github.com/KaiNorberg/SCON/blob/main/scon/atom.h) for more information on atoms.*

Many additional optimization techniques are used, for example, [Computed Gotos](https://eli.thegreenplace.net/2012/07/12/computed-goto-for-efficient-dispatch-tables), [setjmp](https://man7.org/linux/man-pages/man3/longjmp.3.html) based error handling to avoid excessive error checking in the hot path, [Tail Call Optimization](https://en.wikipedia.org/wiki/Tail_call) and much more.

*See [eval.h](https://github.com/KaiNorberg/SCON/blob/main/scon/eval.h) for more information on the evaluator.*

## Benchmarks

Included below are a handful of benchmarks comparing SCON with python 3.14.3 and Lua 5.4.8 using hyperfine, all benchmarks were performed in Fedora 43 (6.19.11-200.fc43.x86_64).

### Fib35

Benchmark to find the 35th Fibonacci number without tail call optimization.

| Command | Mean [ms] | Min [ms] | Max [ms] | Relative |
|:---|---:|---:|---:|---:|
| `scon bench/fib35.scon` | 550.2 ± 11.0 | 535.5 | 572.8 | 1.00 |
| `lua bench/fib35.lua` | 826.8 ± 38.6 | 769.7 | 900.2 | 1.50 ± 0.08 |
| `python bench/fib35.py` | 1109.4 ± 14.7 | 1085.3 | 1136.0 | 2.02 ± 0.05 |

### Fib65

Benchmark to find the 65th Fibonacci number with tail call optimization.

| Command | Mean [µs] | Min [µs] | Max [µs] | Relative |
|:---|---:|---:|---:|---:|
| `scon bench/fib65.scon` | 613.9 ± 90.6 | 535.1 | 3310.0 | 1.00 |
| `lua bench/fib65.lua` | 1049.5 ± 165.0 | 920.3 | 2663.3 | 1.71 ± 0.37 |
| `python bench/fib65.py` | 13155.4 ± 1254.2 | 11688.3 | 23926.9 | 21.43 ± 3.76 |

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

symbol = sign | "!" | "$" | "%" | "&" | "*" | "." | "/" | ":" | "<" | "=" | ">" | "?" | "@" | "^" | "_" | "|" | "~" | "{" | "}" | "[" | "]" | "#" | "," | ";" | "`" ;
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

## Evaluation

A parsed SCON expression can optionally be evaluated. This means that everything described below is an optional extension
of the language, not a core part of it, and should be considered as such. This can be quite convenient as it allows SCON to be utilized as a simple and efficient markup language similar to JSON or XML.

Evaluation is the process of recursively reducing an expression to its simplest form in a depth-first, left-to-right
manner.

### Callables

There are three callable types, intrinsics, natives and lambdas. Lambdas are defined in SCON, natives are defined in C and intrinsics are handled by the bytecode compiler.

```ebnf
callable = unquoted_atom | lambda ;
```

### Atoms

There are no integers, floats or similar within SCON, only atoms with different "shapes".

An atom can be either string-shaped, integer-shaped or float-shaped, for convenience an atom that is integer-shaped or float-shaped is also considered number-shaped.

```ebnf
string = quoted_atom ;

number = integer | float ;
integer = decimal_integer | hex_integer | octal_integer | binary_integer ;

decimal_integer = [ sign ], digit, { [ "_" ], digit } ;
hex_integer = [ sign ], "0", ( "x" | "X" ), hex, { [ "_" ], hex } ;
octal_integer = [ sign ], "0", ( "o" | "O" ), octal, { [ "_" ], octal } ;
binary_integer = [ sign ], "0", ( "b" | "B" ), binary, { [ "_" ], binary } ;

float = float_number | float_naked_decimal | float_trailing_decimal | scientific_number | special_float ;

float_number = [ sign ], digit, { [ "_" ], digit }, ".", digit, { [ "_" ], digit }, [ ( "e" | "E" ), [ sign ], digit, { [ "_" ], digit } ] ;
float_naked_decimal = [ sign ], ".", digit, { [ "_" ], digit }, [ ( "e" | "E" ), [ sign ], digit, { [ "_" ], digit } ] ;
float_trailing_decimal = [ sign ], digit, { [ "_" ], digit }, ".", [ ( "e" | "E" ), [ sign ], digit, { [ "_" ], digit } ] ;
scientific_number = [ sign ], digit, { [ "_" ], digit }, ( "e" | "E" ), [ sign ], digit, { [ "_" ], digit } ;
special_float = ( [ sign ], ( "i" | "I" ), ( "n" | "N" ), ( "f" | "F" ) ) | ( ( "n" | "N" ), ( "a" | "A" ), ( "n" | "N" ) ) ;
```

All mathematical operations on integer-shaped or float-shaped atoms follow C semantics.

#### Type Coercion

For any math intrinsic that takes in multiple arguments, C-like type promotion rules are used.

This means that if any of the atoms provided to the operation are float-shaped, the others will also be converted to floats. Otherwise, they will be converted to integers.

If one or more of the atoms are neither integer-shaped nor float-shaped, the evaluation will fail.

### Lists

During the evaluation of a list, the first item is immediately evaluated and if it evaluates to a callable item, it will be executed with the remaining items in the list as its arguments and with the list being replaced by the result of the evaluation.

Certain primitives may not evaluate all items within the list, for example, the `or` intrinsic will stop evaluating on the first truthy item. Most primitives, such as `+`, will evaluate all items.

### Truthiness

The following grammar defines truthy and falsy items:

```ebnf
truthy = true | expression - falsy_item ;
falsy  = falsy_list | falsy_atom | zero ;

falsy_list = nil ;
falsy_atom = '""' | false ;

true = "1" ;
false = "0" ;

nil = "(", { white_space }, ")" ;

zero = decimal_zero | hex_zero | octal_zero | binary_zero ;
decimal_zero = [ sign ], "0", { [ "_" ], "0" }, [ ".", { [ "_" ], "0" } ], [ ( "e" | "E" ), [ sign ], digit, { [ "_" ], digit } ] ;
hex_zero     = [ sign ], "0", ( "x" | "X" ), "0", { [ "_" ], "0" } ;
octal_zero   = [ sign ], "0", ( "o" | "O" ), "0", { [ "_" ], "0" } ;
binary_zero  = [ sign ], "0", ( "b" | "B" ), "0", { [ "_" ], "0" } ;
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
intrinsic and can be accessed using their names.

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

| Constant | Value |
|----------|-------|
| `true`   | `1` |
| `false`  | `0` |
| `nil`    | `()` |
| `pi`     | `3.14159265358979323846` |
| `e`      | `2.7182818284590452354` |

### Intrinsics

#### Core & Evaluation

**`(quote <expression>) -> <expression>`**

Returns the provided expression without evaluating it.

**`(list {expression} ) -> <list>`**

Returns a new list containing the results of evaluating each expression.

**`(do <expression> {expression}) -> <item>`**

Evaluates each expression in sequence and returns the result of the last one.

**`(lambda ( {arg: atom} ) <expression> {expression} ) -> <lambda>`**

Returns a user-defined anonymous function. When called, the body expressions are evaluated in sequence, and the result of the last expression is returned.

---

#### Variables & Scope

**`(def <name: atom> <value: item>) -> <value: item>`**
  
Defines a variable with the given name and value within the current scope.

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

Evaluates arguments left-to-right, returning the last truthy value, or the first falsy value if any are falsy. Will stop evaluating arguments as soon as one is falsy.

**`(or <item> {item}) -> <item>`**

Evaluates arguments left-to-right, returning the first truthy value, or the last falsy value if all are falsy. Will stop evaluating arguments as soon as one is truthy.

**`(not <item>) -> <true|false>`**

Returns `true` if the argument is falsy, otherwise `false`.

---

#### Arithmetic

**`(+ <number> {number}) -> <number>`**

Returns the sum of all arguments. If only one argument is provided, returns its self.

**`(- <number> {number}) -> <number>`**

Returns the result of subtracting the sum of all arguments from the first argument. If only one argument is provided, returns its negation.

**`(* <number> {number}) -> <number>`**

Returns the product of all arguments. If only one argument is provided, returns its self.

**`(/ <number> {number}) -> <number>`**

Returns the result of dividing the first argument by the subsequent arguments. If only one argument is provided, returns its reciprocal.

If a division by zero occurs, the evaluation fails.

**`(% <number> <number>) -> <number>`**

Returns the remainder of dividing the first argument by the second (modulo).

If a division by zero occurs, the evaluation fails.

---

#### Comparison

**`(= <item> <item> {item}) -> <true|false>`**

Returns `true` if all arguments are equal (numerically if all are numbers, otherwise by string comparison), otherwise `false`.

Will stop evaluating arguments as soon as one is not equal.

**`(!= <item> <item> {item}) -> <true|false>`**

Returns `true` if any arguments are not equal, otherwise `false`.

Will stop evaluating arguments as soon as one is equal.

**`(== <item> <item> {item}) -> <true|false>`**

Returns `true` if all arguments are exactly equal using string comparison, otherwise `false`.

Will stop evaluating arguments as soon as one is not equal.

**`(!== <item> <item> {item}) -> <true|false>`**

Returns `true` if any arguments are not exactly equal using string comparison, otherwise `false`.

Will stop evaluating arguments as soon as one is equal.

**`(< <item> <item> {item}) -> <true|false>`**

Returns `true` if each argument is less than the next, otherwise `false`.

Will stop evaluating arguments as soon as one is not less than the next.

**`(<= <item> <item> {item}) -> <true|false>`**

Returns `true` if each argument is less than or equal to the next, otherwise `false`.

Will stop evaluating arguments as soon as one is not less than or equal to the next.

**`(> <item> <item> {item}) -> <true|false>`**

Returns `true` if each argument is greater than the next, otherwise `false`.

Will stop evaluating arguments as soon as one is not greater than the next.

**`(>= <item> <item> {item}) -> <true|false>`**

Returns `true` if each argument is greater than or equal to the next, otherwise `false`.

Will stop evaluating arguments as soon as one is not greater than or equal to the next.

---

#### Bitwise

**`(& <integer> <integer> {integer}) -> <integer>`**

Returns the bitwise AND of all arguments.

**`(| <integer> <integer> {integer}) -> <integer>`**

Returns the bitwise OR of all arguments.

**`(^ <integer> <integer> {integer}) -> <integer>`**

Returns the bitwise XOR of all arguments.

**`(~ <integer>) -> <integer>`**

Returns the bitwise NOT of the argument.

**`(<< <val: integer> <shift: integer>) -> <integer>`**

Returns the value bitwise shifted left.

**`(>> <val: integer> <shift: integer>) -> <integer>`**

Returns the value bitwise shifted right.

---

### Standard Library

Since SCON is a functional language, side effects should be avoided when possible. As such, any native with side effects will be suffixed with an exclamation mark `!`.

#### Error Handling

**`(assert! <cond: item> <msg: item>) -> <cond: item>`**

Evaluates `<cond>`. If it is falsy, the evaluation fails and throws an error with `<msg>` as the message.

**`(throw! <msg: atom>)`**

Throws an error with the given atom being the error message.

---

#### Functions & Higher-Order

**`(map <callable> <list>) -> <list>`**

Returns a new list by applying `<callable>` to each item in `<list>`. The `<callable>` must accept a single argument.

**`(filter <callable> <list>) -> <list>`**

Returns a new list containing only items from `<list>` for which `<callable>` returns a truthy value. The `<callable>` must accept a single argument.

**`(reduce <callable> <initial> <list>) -> <item>`**

Reduces `<list>` to a single value. The `<callable>` must accept two arguments: the `accumulator` (which starts as `<initial>`) and the current `item`, and it should return the new accumulator value.

**`(sort <list> [callable]) -> <list>`**

Returns a new list containing the sorted items of the list. The sort is guaranteed to be stable.

If a `callable` is provided, it should accept two arguments and return a truthy value if the first argument should appear before the second.

If no `callable` is specified, then items are sorted in ascending order.

---

#### Sequences (Lists & Strings)

**`(concat {item}) -> <item>`**

Returns a new atom or list by concatenating all items, can also be utilized for "append" or "prepend" operations. If any of the items is a list, the result will be a list, otherwise it will be an atom.

**`(first <item>) -> <atom>`**

Returns the first item of a list or the first character of an atom as a new atom.

**`(last <item>) -> <atom>`**

Returns the last item of a list or the last character of an atom as a new atom.

**`(rest <item>) -> <item>`**

Returns a new list containing all except the first item of a list or an atom containing all except the first character of an atom.

**`(init <item>) -> <item>`**

Returns a new list containing all but the last item of a list or an atom containing all but the last character of an atom.

**`(nth <item> <n: number>) -> <atom>`**

Returns the n-th item of a list or the n-th character of an atom as a new atom, if n is negative, it returns the n-th item from the end.

If the index is out of bounds, returns nil.

**`(index <item> <subitem: item>) -> <number>`**

Returns the index of the first occurrence of the subitem in the item.

If the subitem is not found, returns nil.

**`(reverse <item>) -> <item>`**

Returns a new list containing the items of `<item>` in reverse order or an atom containing the characters of `<item>` in reverse order.

**`(slice <item> <start: number> [end: number]) -> <item>`**

Returns a sub-list or sub-atom of `<item>` starting from the `<start>` index to the `<end>` index. If `<end>` is not provided, it slices to the end of the item. Negative indices can be used to count from the end.

---

#### String Manipulation

**`(starts-with? <item> <prefix: item>) -> <true|false>`**

Returns `true` if the provided item is an atom that starts with the prefix or a list whose first item is the prefix, otherwise `false`.

**`(ends-with? <item> <suffix: item>) -> <true|false>`**

Returns `true` if the provided item is an atom that ends with the suffix or a list whose last item is the suffix, otherwise `false`.

**`(contains? <item> <subitem: item>) -> <true|false>`**

Returns `true` if the provided item is an atom that contains the subitem as a substring or a list that contains an item equal to the subitem, otherwise `false`.

**`(replace <item> <old: item> <new: item>) -> <item>`**

Returns a new atom or list with all occurrences of `<old>` replaced by `<new>`.

**`(join <list> <separator: string>) -> <string>`**

Returns a new atom created by joining all items in `<list>` with `<separator>`.

**`(split <atom: string> <separator: string>) -> <list>`**

Returns a new list by splitting `<atom>` into sub-atoms at each occurrence of `<separator>`.

**`(upper <atom: string>) -> <string>`**

Returns a new atom with all characters converted to uppercase.

**`(lower <atom: string>) -> <string>`**

Returns a new atom with all characters converted to lowercase.

**`(trim <atom: string>) -> <string>`**

Returns a new atom with leading and trailing whitespace removed.

---

#### Introspection

**`(len <item> {item}) -> <number>`**

Returns the total number of items in the lists and the number of characters in the atoms for all provided arguments.

**`(atom? <item> {item}) -> <true|false>`**

Returns `true` if all items are atoms, otherwise `false`.

**`(int? <item> {item}) -> <true|false>`**

Returns `true` if all items are integer shaped atoms, otherwise `false`.

**`(float? <item> {item}) -> <true|false>`**

Returns `true` if all items are float shaped atoms, otherwise `false`.

**`(number? <item> {item}) -> <true|false>`**

Returns `true` if all items are integer or float shaped atoms, otherwise `false`.

**`(lambda? <item> {item}) -> <true|false>`**

Returns `true` if all items are lambdas, otherwise `false`.

**`(native? <item> {item}) -> <true|false>`**

Returns `true` if all items are natives, otherwise `false`.

**`(callable? <item> {item}) -> <true|false>`**

Returns `true` if all items are lambdas or natives, otherwise `false`.

**`(list? <item> {item}) -> <true|false>`**

Returns `true` if all items are lists, otherwise `false`.

**`(empty? <item> {item}) -> <true|false>`**

Returns `true` if all items are empty lists `()`, or empty atoms `""`, otherwise `false`.

---

#### Type Casting

**`(int <atom>) -> <number>`**

Returns the integer shaped representation of the atom.

**`(float <atom>) -> <number>`**

Returns the float shaped representation of the atom.

**`(string <item>) -> <string>`**

Returns the stringified representation of the item.

---

#### Association Lists (Dictionaries)

**`(get <list> <name: item>) -> <item>`**

Returns the second item of the first sub-list whose first item evaluates to `<name>`.

**`(keys <list>) -> <list>`**

Returns a new list containing the first item of every sub-list.

**`(values <list>) -> <list>`**

Returns a new list containing the second item of every sub-list.

**`(assoc <list> <key: item> <value: item>) -> <list>`**

Returns a new list with the sub-list whose first item is `<key>` having the second item replaced or added with `<value>`.

**`(dissoc <list> <key: item>) -> <list>`**

Returns a new list with the sub-list whose first item is `<key>` removed.

**`(update <list> <key: item> <lambda>) -> <list>`**

Returns a new list with the sub-list whose first item is `<key>` having its second item updated by applying `<lambda>` to it.

---

#### System & Environment

**`(include <path: string>) -> <item>`**

Returns the result of evaluating the SCON file at the given path, variables defined in the included file will be available in the current scope.

**`(read-file <path: string>) -> <string>`**

Reads the file at the given path and returns its contents as a raw string atom without evaluating it.

**`(print! {item}) -> nil`**

Prints the string representation of all arguments to the standard output.

**`(println! {item}) -> nil`**

Prints the string representation of all arguments to the standard output, followed by a newline.

**`(format <format: string> {item}) -> <string>`**

Returns a formatted string using python-like formatting, where `{}` is used as a placeholder for the provided arguments.
Positional arguments can be used to specify the index of the argument to be used, for example `{0}`.

```lisp
(format "Hello, {}!" "World") // Returns "Hello, World!"
(format "{1} {0}" "World" "Hello") // Returns "Hello World"
```

**`(time) -> <number>`**

Returns the current time in seconds since the Unix epoch.

**`(env <name: string>) -> <string>`**

Returns the value of the environment variable as an atom, or an empty string if it is not set.

---

#### Math & Trigonometry

**`(min <val: number> {number}) -> <number>`**

Returns the smallest of all arguments.

**`(max <val: number> {number}) -> <number>`**

Returns the largest of all arguments.

**`(clamp <val: number> <min: number> <max: number>) -> <number>`**

Restricts a value to be between the given minimum and maximum.

**`(abs <val: number>) -> <number>`**

Returns the absolute value of the argument.

**`(floor <val: number>) -> <number>`**

Returns the largest integer less than or equal to the argument.

**`(ceil <val: number>) -> <number>`**

Returns the smallest integer greater than or equal to the argument.

**`(round <val: number>) -> <number>`**

Returns the argument rounded to the nearest integer.

**`(pow <base: number> <exp: number>) -> <number>`**

Returns the base raised to the power of the exponent.

**`(log <val: number> [base: number]) -> <number>`**

Returns the natural logarithm of the argument, or the specified logarithm of the argument.

**`(sqrt <val: number>) -> <number>`**

Returns the square root of the argument.

**`(sin <val: number>) -> <number>`**

Returns the sine of the argument.

**`(cos <val: number>) -> <number>`**

Returns the cosine of the argument.

**`(tan <val: number>) -> <number>`**

Returns the tangent of the argument.

**`(asin <val: number>) -> <number>`**

Returns the arcsine of the argument.

**`(acos <val: number>) -> <number>`**

Returns the arccosine of the argument.

**`(atan <val: number>) -> <number>`**

Returns the arctangent of the argument.

**`(atan2 <y: number> <x: number>) -> <number>`**

Returns the arctangent of the quotient of its arguments.

**`(sinh <val: number>) -> <number>`**

Returns the hyperbolic sine of the argument.

**`(cosh <val: number>) -> <number>`**

Returns the hyperbolic cosine of the argument.

**`(tanh <val: number>) -> <number>`**

Returns the hyperbolic tangent of the argument.

**`(asinh <val: number>) -> <number>`**

Returns the inverse hyperbolic sine of the argument.

**`(acosh <val: number>) -> <number>`**

Returns the inverse hyperbolic cosine of the argument.

**`(atanh <val: number>) -> <number>`**

Returns the inverse hyperbolic tangent of the argument.

**`(rand <min: number> <max: number>) -> <number>`**

Returns a random number between the given range.

**`(seed! <val: number>)`**

Seeds the random number generator.
