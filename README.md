# Reduct

<br>
<div align="center">
    <a href="https://github.com/KaiNorberg/Reduct/issues">
      <img src="https://img.shields.io/github/issues/KaiNorberg/Reduct">
    </a>
    <a href="https://github.com/KaiNorberg/Reduct/stargazers">
      <img src="https://img.shields.io/github/stars/KaiNorberg/Reduct">
    </a>
    <a href="https://kainorberg.github.io/Reduct/html/index.html">
      <img src="https://img.shields.io/badge/docs-Doxygen-blue">
    </a>
    <a href="https://github.com/KaiNorberg/Reduct/blob/main/license">
      <img src="https://img.shields.io/github/license/KaiNorberg/Reduct">
    </a>
    <a href="https://github.com/KaiNorberg/Reduct/actions/workflows/test.yml">
        <img src="https://github.com/KaiNorberg/Reduct/actions/workflows/test.yml/badge.svg" alt="Test"/>
    </a>
    <a href="https://github.com/KaiNorberg/Reduct/releases">
      <img src="https://img.shields.io/github/v/release/KaiNorberg/Reduct">
    </a>
    <br>
</div>
<br>

Reduct is a functional, immutable, S-expression based configuration and scripting language. It aims to combine the flexibility of a Lisp with the ease-of-use and performance of a language like Lua. All within a C99 header-only library.

## Tools

### Command Line Interface (CLI)

A simple CLI tool is provided to evaluate Reduct files or expressions directly from the terminal.

```bash
reduct my_file.rdt
reduct -e "(+ 1 2 3)"
reduct -d my_file.rdt # output the compiled bytecode
reduct -s my_file.rdt # silent mode, wont output the result of the evaluation
```

#### Setup

You can download pre-built binaries for Linux, macOS, and Windows from the [Releases](https://github.com/KaiNorberg/Reduct/releases) page.

Alternatively, you can build it from source:

```bash
cd tools/reduct-cli
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

To install the tool write:

```bash
cmake --install build # if that fails: sudo cmake --install build
```

### Visual Studio Code

A syntax highlighting extension for Visual Studio Code can be found at `tools/reduct-vscode/`.

#### Setup

The extension can be found on the [marketplace](https://marketplace.visualstudio.com/items?itemName=KaiNorberg.reduct).

Alternatively, to install it manually, copy the `tools/reduct-vscode/` directory to `%USERPROFILE%\.vscode\extensions\` if you're on Windows or `~/.vscode/extensions/` if you're on macOS/Linux.

Finally, restart Visual Studio Code.

## Introduction

Within Reduct, all expressions are either atoms or lists, where an atom is a sequence of characters and a list is a sequence of expressions enclosed in parentheses.

For example:

```lisp
("atom1" atom2 3.14 (4atom 5678)) // A list containing three atoms and a list.
```

The tree structure that this creates is called an Abstract Syntax Tree (AST), and for some use cases, that will be enough.

The evaluation of an AST is an optional part of the language.

### Evaluation

Evaluation is the process of recursively reducing an AST into another, simpler, AST, much like how mathematical expressions are reduced to their final values.

For example:

```lisp
(+ 1 2) // Evaluates to "3"
(* 3 4) // Evaluates to "12"
```

> Note how the top level expression is implicitly a `do` block, with the last evaluation being returned.

### Atoms

Atoms are the most basic building blocks of Reduct. They are sequences of characters (strings) that can represent text, numbers, or symbols.

```lisp
"This is a string atom"
12345 // An integer-shaped atom
3.14159 // A float-shaped atom
my-variable // A symbol-shaped atom
```

There are no explicit integers or floats within Reduct, only atoms with different "shapes".

An atom can be either string-shaped, integer-shaped or float-shaped, for convenience an atom that is integer-shaped or float-shaped is also considered number-shaped.

### Callables

There are three callable types: intrinsics, natives, and lambdas. Lambdas are defined in Reduct, natives are defined in C, and intrinsics are handled by the bytecode compiler.

```lisp
(lambda (n) (* n n)) // A lambda that squares its argument.
```

To call a callable, use it as the first item in a list:

```lisp
((lambda (n) (* n n)) 5) // Evaluates to "25"
```

### Hello World

We should now know enough to write a basic "Hello World" script.

An obvious way to write "Hello World" in Reduct might look like:

```lisp
(println! "Hello, World!")
```

However, since Reduct is all about reduction, it could be even simpler. We could simply write:

```lisp
"Hello, World!"
```

Which will, of course, evaluate to `Hello, World!`.

For more examples, see the `bench/` and `tests/` directories.

## Motivation

Reduct aim to provide a series of potential advantages that over existing languages.

### Performance

Reduct is designed to be fast and efficient, utilizing a register-based VM and many other techniques to improve performance, allowing it to outperform even Lua (though no the JIT version obviously, maybe one day...), in some benchmarks.

See [Benchmarks](#benchmarks) for more information.

### Lisps and Readability

Lisps, which Reduct takes heavy inspiration from, are generally agreed to be a very powerful style of language and yet are frequently criticized for their poor readability.

The most often blamed source of this poor readability is the sheer volume of parentheses.

Reduct makes the argument that most of these complaints are due to nesting, not parentheses; that it can be solved via infix notation, banning `let` and a more modern style guide.

Included are three examples of a basic program written in common Lisp C, and Reduct.

#### Lisp

```lisp
(let ((x 10)
      (y 20))
  (let ((z (+ x y)))
    (* z 2)))
```

#### C

```c
int main()
{
    int x = 10;
    int y = 20;
    int z = x + y;
    return z * 2;
}
```

#### Reduct

```lisp
(do
    (def x 10)
    (def y 20)
    (def z {x + y})
    {z * 2}
)
```

Note how in Reduct the use of curly braces for infix notation and the `def` intrinsic for scoped definitions allows for a more familiar, imperative-like structure while remaining entirely functional, and S-expression based.

### Flexibility

Since code and data are represented using the same structures, Reduct can be used for practically anything.

One of Reducts original intended use cases was for a C UI library, where it would act as both the markup and scripting language, almost like a combination of HTML, CSS and JavaScript.

Included is an example of what that might look like:

```lisp
(def my-color (lambda ()
    (if {(uptime!) % 1000 < 500}
        "red"
        "blue"
    )
))  

("button"
    ("label" "Click me!")
    ("on-click" (lambda () (println! "Button clicked!")))
    ("style" (
        ("color" (my-color))
        ("width" 100)
        ("height" 50)
    ))
)
```

Despite that original use case, Reduct can just as easily be utilized for scripting, configuration, or even as a general-purpose embedded language.

## Additional Concepts

### Type Coercion

For any math intrinsic that takes in multiple arguments, C-like type promotion rules are used.

This means that if any of the atoms provided to the operation are float-shaped, the others will also be converted to floats. Otherwise, they will be converted to integers.

If one or more of the atoms are neither integer-shaped nor float-shaped, the evaluation will fail.

For example:

```lisp
(+ 1 2.5) // Evaluates to "3.5"
(* 2 3) // Evaluates to "6"
(/ 10 3) // Evaluates to "3"
(/ 10 3.0) // Evaluates to "3.333333"
```

### Truthiness

A truthy item is any item that is not falsy. Falsy items include `nil` (an empty list), an empty atom, the constant `false` (which evaluates to `0`), or any number-shaped atom that evaluates to zero.

For example:

```lisp
(if () "truthy" "falsy") // Evaluates to "falsy"
(if 0 "truthy" "falsy") // Evaluates to "falsy"
(if "" "truthy" "falsy") // Evaluates to "falsy"
(if 1 "truthy" "falsy") // Evaluates to "truthy"
(if false "truthy" "falsy") // Evaluates to "falsy"
(if true "truthy" "falsy") // Evaluates to "truthy"
```

### Ordering

Reduct defines a total ordering for all possible items. This is used by comparison primitives like `<` or `sort`.

The ordering of types is defined as:

```plaintext
number < string < list
```

The following rules apply:

- **number:** Compared by value, a greater value is considered larger. If both values are equal, but one is float-shaped and the other is integer-shaped, the float-shaped atom is considered larger.
- **string:** Compared lexicographically by their ASCII characters, or by their length if one is a prefix of the other.
- **list:** Compared item by item. A list is considered "less than" another if its first non-equal item is lesser than the other list's first non-equal item, or by their length if one is a prefix of the other.

For example, all following expressions are true:

```lisp
(< 1 2)
(< 1 2.5)
(< "apple" "banana")
(< "apple" "apples")
(< (1 2) (1 3))
(< (1 2) (1 2 3))
```

### Variables

Variables are defined using the `def` intrinsic and can be accessed using their names. Just like everything else, variables are immutable and as such there is no `set` or similar intrinsic.

As an example, variables can be used to create a more traditional "function definition" by defining a variable as a lambda:

```lisp
(def add (lambda (a b) (+ a b)))

(add 1 2) // Evaluates to "3"
```

### Infix Expressions

Infix expressions provide a more convenient way to write certain expressions, particularly mathematical or logical expressions.

To write an infix expression, use curly braces:

```lisp
{1 + 2 * 3} // (1 + (2 * 3))
{2 * (my-func1 1 2)} // (2 * (my-func 1 2))
{not (my-func2 data) or {count > 10}} // (or (not (my-func2 data)) (> count 10))
```

When the parser encounters an infix expression it will expand it into a normal expression, using the following rules:

1. Operators are grouped based on their precedence. For example, `*` and `/` have higher precedence than `+` and `-`.
2. Operators with the same precedence are evaluated from left to right.
3. If an expression is wrapped in another infix expression, it is treated as a single unit and its precedence is ignored (e.g., `{{1 + 2} * 3}`).
4. Lists within an infix expression are treated as usual.
5. If an infix expression contains only a single item, it is treated as that item (e.g., `{1}` becomes `1`).

The following table lists the supported operators:

| Precedence | Operators |
| --- | --- |
| 7 | `not`, unary `-` |
| 6 | `*`, `/`, `%` |
| 5 | `+`, `-` |
| 4 | `<<`, `>>` |
| 3 | `&`, `\|`, `^` |
| 2 | `==`, `!=`, `<`, `<=`, `>`, `>=` |
| 1 | `and` |
| 0 | `or` |

## Style Guide

There is no strictly enforced style guide, however, here are some recommendations.

### Indentation

Use 4-space indentation.

### Parentheses

If a list spans multiple lines, the closing parentheses should be on its own line, aligned with the opening parentheses. If a list is not spanned across multiple lines, the closing parentheses should be on the same line as the last item.

> The argument for placing parentheses on their own lines, breaking with Lisp tradition, is to allow for multiline lists to be easily copy, pasted and cut,

For example:

```lisp
(def fib (lambda (n) 
    (if (<= n 1)
        n
        (+ (fib (- n 1)) (fib (- n 2)))
    )
))
```

### Naming

Use `kebab-case` for variable and function names.

```lisp
(def my-variable 10)

(def my-function (lambda (x y)
    (+ x y)
)) 
```

## C API

Included is an example of using Reduct as a single header without linking:

```c
// my_file.c

#define REDUCT_INLINE
#include "reduct.h"

#include <stdio.h>
#include <stdlib.h>

char buffer[0x10000];

int main(int argc, char **argv)
{    
    reduct_t* reduct = NULL;

    reduct_error_t error = REDUCT_ERROR();
    if (REDUCT_ERROR_CATCH(&error)) // Setup setjmp.h based error handler.
    {
        // We will return to here if an error occurs.
        reduct_error_print(&error, stderr);
        reduct_free(reduct);
        return 1;
    }

    reduct = reduct_new(&error);

    reduct_stdlib_register(reduct, REDUCT_STDLIB_ALL);

    reduct_handle_t result = reduct_eval_file(reduct, "my_file.rdt");

    reduct_stringify(reduct, &result, buffer, sizeof(buffer));
    printf("%s\n", buffer);

    reduct_free(reduct);
    return 0;
}
```

Included is another example of using Reduct with linking where an additional implementation file is used to build the Reduct parser and evaluator:

```c
// my_file.c

#include "reduct.h"

#include <stdio.h>
#include <stdlib.h>

char buffer[0x10000];

int main(int argc, char **argv)
{
    /// Same as above...
}
```

```c
// my_reduct.c

#define REDUCT_IMPL
#include "reduct.h"
```

### C Natives

Reduct allows for the registration of C functions as "natives" that can be called from within Reduct.

To register a native function, use the `reduct_native_register` function:

```c
reduct_handle_t my_native(reduct_t* reduct, reduct_size_t argc, reduct_handle_t* argv)
{
    // ...
    return reduct_handle_nil(reduct);
}

// ...

reduct_native_t natives[] = {
    {"my-native", my_native},
};
reduct_native_register(reduct, natives, sizeof(natives) / sizeof(reduct_native_t));
```

All Reduct standard library functions are available in C, for example, `reduct_is_atom()`, `reduct_assoc()`, etc.

### Documentation

For more information, either consult the doxygen documentation within the headers or the generated documentation found [here](https://kainorberg.github.io/Reduct//html/index.html).

## Implementation

Reduct is implemented as a register-based bytecode language, where the Reduct source is first parsed into an Abstract Syntax Tree (AST) and then compiled into a custom bytecode format before being executed by the virtual machine/evaluator.

> Note that the "Abstract Syntax Tree" is just a Reduct expression, lists and atoms, meaning that the compiler is itself written to operate on the same data structures as the evaluator produces.

The bytecode format itself is a stream of 32bit instructions, with all instructions able to read/write to an array of registers, or read from an array of constants.

*See [inst.h](https://github.com/KaiNorberg/Reduct/blob/main/reduct/inst.h) for more information on instructions.*

Since Reduct is immutable, the constants array is also used for "captured" values from outer scopes (closures) and we can also allow the compiler to fold constant expressions at compile-time, far more than would normally be possible.

*See [compile.h](https://github.com/KaiNorberg/Reduct/blob/main/reduct/compile.h) for more information on the compiler.*

To improve caching and reduce pointer indirection, Reduct uses "handles" (`reduct_handle_t`) which are [Tagged Pointers](https://en.wikipedia.org/wiki/Tagged_pointer) using NaN boxing to allow a single 64bit value to store either a 48 bit signed integer, IEEE 754 double or a pointer to a heap allocated item.

*See [handle.h](https://github.com/KaiNorberg/Reduct/blob/main/reduct/handle.h) for more information on handles.*

Items (`reduct_item_t`) represent all heap allocated objects, such as lists, atoms and closures. All items are exactly 64 bytes in size and allocated using a custom pool allocator and freed using a garbage collector and free list.

Since Reduct uses its handles to store most integers and floats, it can avoid heap allocations for many common values, significantly reducing the pressure on the garbage collector and improving caching.

*See [item.h](https://github.com/KaiNorberg/Reduct/blob/main/reduct/item.h) for more information on items.*

Lists are implemented as a "bit-mapped vector trie", providing $O(log_{w} n)$ access, insertion, and deletion, where $w$ is the width of each node in the trie.

*See [list.h](https://github.com/KaiNorberg/Reduct/blob/main/reduct/list.h) for more information on lists.*

All atoms use [String Interning](https://en.wikipedia.org/wiki/String_interning), meaning that every unique atom is only stored once in memory. This makes any string comparison into a single pointer comparison, and it means that parsing the integer/floating point value of an atom or an items truthiness only needs to be done once.

*See [atom.h](https://github.com/KaiNorberg/Reduct/blob/main/reduct/atom.h) for more information on atoms.*

Many additional optimization techniques are used, for example, [Computed Gotos](https://eli.thegreenplace.net/2012/07/12/computed-goto-for-efficient-dispatch-tables), [setjmp](https://man7.org/linux/man-pages/man3/longjmp.3.html) based error handling to avoid excessive error checking in the hot path, [Tail Call Optimization](https://en.wikipedia.org/wiki/Tail_call) and much more.

*See [eval.h](https://github.com/KaiNorberg/Reduct/blob/main/reduct/eval.h) for more information on the evaluator.*

## Benchmarks

Included below are a handful of benchmarks comparing Reduct with python 3.14.3 and Lua 5.4.8 using hyperfine, all benchmarks were performed in Fedora 43 (6.19.11-200.fc43.x86_64).

### Fib35

Finds the 35th Fibonacci number without tail call optimization.

| Command | Mean [ms] | Min [ms] | Max [ms] | Relative |
|:---|---:|---:|---:|---:|
| `reduct bench/fib35.rdt` | 550.2 ± 11.0 | 535.5 | 572.8 | 1.00 |
| `lua bench/fib35.lua` | 826.8 ± 38.6 | 769.7 | 900.2 | 1.50 ± 0.08 |
| `python bench/fib35.py` | 1109.4 ± 14.7 | 1085.3 | 1136.0 | 2.02 ± 0.05 |

For this benchmark, memory usage was also tracked using `heaptrack`:

| Command | Peak Memory [MB] |
|:---|---:|
| `reduct bench/fib35.rdt` | 0.097 |
| `lua bench/fib35.lua` | 0.099 |
| `python bench/fib35.py` | 1.8 |

### Fib65

Finds the 65th Fibonacci number with tail call optimization.

| Command | Mean [µs] | Min [µs] | Max [µs] | Relative |
|:---|---:|---:|---:|---:|
| `reduct bench/fib65.rdt` | 613.9 ± 90.6 | 535.1 | 3310.0 | 1.00 |
| `lua bench/fib65.lua` | 1049.5 ± 165.0 | 920.3 | 2663.3 | 1.71 ± 0.37 |
| `python bench/fib65.py` | 13155.4 ± 1254.2 | 11688.3 | 23926.9 | 21.43 ± 3.76 |

### Brainfuck

A simple jump-table optimized Brainfuck interpreter that runs a "Hello World!" program.

> This benchmark also acts as a fun Turing completeness proof.

| Command | Mean [µs] | Min [µs] | Max [µs] | Relative |
|:---|---:|---:|---:|---:|
| `reduct bench/brainfuck.rdt` | 794.3 ± 101.8 | 720.6 | 1779.7 | 1.00 |
| `lua bench/brainfuck.lua` | 1112.1 ± 146.5 | 1022.6 | 2359.5 | 1.40 ± 0.26 |

For this benchmark, memory usage was also tracked using `heaptrack`:

| Command | Peak Memory [MB] |
|:---|---:|
| `reduct bench/brainfuck.rdt` | 0.185 |
| `lua bench/brainfuck.lua` | 0.102 |

### Mandelbrot

Outputs an 80 by 40 visualization of the Mandelbrot set with 10000 iterations.

| Command | Mean [ms] | Min [ms] | Max [ms] | Relative |
|:---|---:|---:|---:|---:|
| `reduct bench/mandelbrot.rdt` | 330.7 ± 8.3 | 318.0 | 347.3 | 1.00 |
| `lua bench/mandelbrot.lua` | 369.2 ± 14.3 | 356.5 | 403.3 | 1.12 ± 0.05 |

## Testing

Testing is done via the `tests/run_tests.sh` script, which is ran via [github actions](https://github.com/KaiNorberg/Reduct/blob/main/.github/workflows/test.yml) on Ubuntu, Windows and macOS.

The script will compile the CLI tool with clang `-fsanitize=address,undefined`, run a set of test scripts found in `tests/` and then use libfuzzer to fuzz the parser, compiler and evaluator.

In addition to the test script, the code base heavily uses assertions to, hopefully, catch any potential bugs or regressions.

## Grammar

The grammar of Reduct is designed to be as straight forward as possible, the full grammar using [EBNF](https://en.wikipedia.org/wiki/Extended_Backus%E2%80%93Naur_form) can be found below.

```ebnf
file = { expression | white_space | comment } ;
expression = item | infix ;
item = list | atom ;

infix =  "{", { expression | white_space | comment }, "}" ;

list = "(", { expression | white_space | comment }, ")" ;

callable = symbol | lambda ;
lambda = "(", "lambda", "(", { symbol }, ")", { expression }, ")" ;

atom = string | symbol | number ;

string = '"', { character | white_space | escape_sequence }, '"' ;
symbol = character, { character } ;

number = integer | float ;
integer = decimal_integer | hex_integer | octal_integer | binary_integer ;

truthy = true | expression - falsy_item ;
falsy  = nil | '""' | false | zero ;

true = "1" ;
false = "0" ;

nil = "(", { white_space }, ")" ;

zero = decimal_zero | hex_zero | octal_zero | binary_zero ;
decimal_zero = [ sign ], "0", { [ "_" ], "0" }, [ ".", { [ "_" ], "0" } ], [ ( "e" | "E" ), [ sign ], digit, { [ "_" ], digit } ] ;
hex_zero     = [ sign ], "0", ( "x" | "X" ), "0", { [ "_" ], "0" } ;
octal_zero   = [ sign ], "0", ( "o" | "O" ), "0", { [ "_" ], "0" } ;
binary_zero  = [ sign ], "0", ( "b" | "B" ), "0", { [ "_" ], "0" } ;

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

comment = line_comment | block_comment ;
line_comment = "//", { character | " " | "\t" }, [ "\n" | "\r" ] ;
block_comment = "/*", { character | white_space }, "*/" ;

white_space = " " | "\t" | "\n" | "\r" ;
escape_sequence = "\\", ( "a" | "b" | "e" | "f" | "n" | "r" | "t" | "v" | "\\" | "'" | '"' | "?" | ( "x", hex, hex ) ) ;

character = letter | digit | punctuation ;

punctuation = sign | "!" | "$" | "%" | "&" | "*" | "." | "/" | ":" | "<" | "=" | ">" | "?" | "@" | "^" | "_" | "|" | "~" | "{" | "}" | "[" | "]" | "#" | "," | ";" | "`" ;
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

### Default Constants

The following constants are defined by default in the Reduct environment:

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

**`(-> <initial: expression> {step: expression}) -> <item>`**

Returns a threaded expression, where the result of each expression is passed as the first argument to the next.

**`(def <name: atom> <value: item>) -> <value: item>`**
  
Defines a variable with the given name and value within the current scope.

Note that there is no `let` intrinsic in Reduct, this is because using `def` within a `do` block accomplishes the same result as a `let` expression in other Lisps:

For example:

```lisp
(do
    (def x 10)
    (def y 20)
    (+ x y)
) // Evaluates to "30"

(let ((x 10) (y 20))
    (+ x y)
) // Evaluates to "30"
```

---

#### Control Flow & Logic

**`(if <cond: item> <then: item> [else: item]) -> <item>`**

Evaluates `<then>` if `<cond>` is truthy, otherwise evaluates `<else>` if provided, otherwise returns `nil`.

**`(cond ( <cond: item> <val: item> ) { ( <cond: item> <val: item> ) }) -> <item>`**

Evaluates each condition in order, returning the value associated with the first truthy condition, or `nil` if none are truthy.

**`(match <target: item> ( <val: item> <res: item> ) { ( <val: item> <res: item> ) }  <default: item>) -> <item>`**

Evaluates the target, then compares it against each case value. Returns the result of the first matching case.

**`(and <item> {item}) -> <item>`**

Evaluates arguments left-to-right, returning the last truthy value, or the first falsy value if any are falsy. Will stop evaluating arguments as soon as one is falsy.

**`(or <item> {item}) -> <item>`**

Evaluates arguments left-to-right, returning the first truthy value, or the last falsy value if all are falsy. Will stop evaluating arguments as soon as one is truthy.

**`(not <item>) -> <true|false>`**

Returns `true` if the argument is falsy, otherwise `false`.

---

#### Binary Operators

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

**`(++ <number>) -> <number>`**

Returns the argument incremented by one.

**`(-- <number>) -> <number>`**

Returns the argument decremented by one.

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

#### Comparison

**`(== <item> <item> {item}) -> <true|false>`**

Returns `true` if all arguments are equal (numerically if all are numbers, otherwise by string comparison), otherwise `false`.

Will stop evaluating arguments as soon as one is not equal.

**`(!= <item> <item> {item}) -> <true|false>`**

Returns `true` if any arguments are not equal, otherwise `false`.

Will stop evaluating arguments as soon as one is equal.

**`(eq? <item> <item> {item}) -> <true|false>`**

Returns `true` if all arguments are exactly equal using string comparison, otherwise `false`.

Will stop evaluating arguments as soon as one is not equal.

**`(ne? <item> <item> {item}) -> <true|false>`**

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

### Standard Library

Since Reduct is a functional language, side effects should be avoided when possible. As such, any native with side effects will be suffixed with an exclamation mark `!`.

#### Error Handling

**`(assert! <cond: item> <msg: item>) -> <cond: item>`**

Evaluates `<cond>`. If it is falsy, the evaluation fails and throws an error with `<msg>` as the message.

**`(throw! <msg: atom>)`**

Throws an error with the given atom being the error message.

**`(try <callable> <catch: lambda>) -> <item>`**

Calls the provided callable. If an error occurs during evaluation, the `<catch>` lambda is executed with the error message as its argument. Otherwise, returns the result of the callable.

---

#### Higher-Order Functions

**`(map <list> <callable>) -> <list>`**

Returns a new list by applying `<callable>` to each item in `<list>`. The `<callable>` must accept a single argument.

**`(filter <list> <callable>) -> <list>`**

Returns a new list containing only items from `<list>` for which `<callable>` returns a truthy value. The `<callable>` must accept a single argument.

**`(reduce <list> [initial: item] <callable>) -> <item>`**

Reduces `<list>` to a single value. The `<callable>` must accept two arguments: the `accumulator` (which starts as `[initial]` or the first item of `<list>` in which case the first item is skipped) and the current `item`, and it should return the new accumulator value.

**`(apply <list> <callable>) -> <item>`**

Calls the callable with the items within the list as its arguments.

**`(any? <list> [callable]) -> <true|false>`**

Returns `true` if any item in the list satisfies the provided `callable` (or is truthy if no callable is provided), otherwise `false`.

**`(all? <list> [callable]) -> <true|false>`**

Returns `true` if all items in the list satisfy the provided `callable` (or are truthy if no callable is provided), otherwise `false`.

**`(sort <list> [callable]) -> <list>`**

Returns a new list containing the sorted items of the list. The sort is guaranteed to be stable.

If a `callable` is provided, it should accept two arguments and return a truthy value if the first argument should appear before the second.

If no `callable` is specified, then items are sorted in ascending order.

---

#### Sequences (Lists & Strings)

**`(len <item> {item}) -> <number>`**

Returns the total number of items in the lists and the number of characters in the atoms for all provided arguments.

**`(range [start: number] <end: number> [step: number]) -> <list>`**

Returns a new list containing a sequence of numbers from `<start>` up to (but not including) `<end>`, incremented by `<step>`. If `<step>` is not provided, it defaults to `1`.

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

**`(nth <item> <n: number> [default: item]) -> <item>`**

Returns the n-th item of a list or the n-th character of an atom as a new atom, if n is negative, it returns the n-th item or character from the end.

If the index is out of bounds, returns `[default]` or `nil`.

**`(assoc <item> <n: number> <value: item> [fill: item]) -> <item>`**

Returns a new list or atom with the n-th element replaced by `<value>`. If n is negative, it replaces the n-th item or character from the end.

If the index is out of bounds and a `fill` value is provided, the list or atom will be extended to accommodate the new value using `fill` as padding. If no `fill` value is provided, an out of bounds access will throw an error.

**`(dissoc <item> <n: number>) -> <item>`**

Returns a new list or atom with the n-th element removed. If n is negative, it removes the n-th item or character from the end.

**`(update <item> <n: number> <callable> [fill: item]) -> <item>`**

Returns a new list or atom with the n-th element updated by applying `<callable>` to it. The `<callable>` must accept a single argument. If n is negative, it updates the n-th item or character from the end.

If the index is out of bounds and a `fill` value is provided, the list or atom will be extended to accommodate the new value using `fill` as padding, and the `callable` will be applied to the `fill` value to produce the new element. If no `fill` value is provided, an out of bounds access will throw an error.

**`(index-of <item> <subitem: item>) -> <number>`**

Returns the index of the first occurrence of the subitem in the item.

If the subitem is not found, returns nil.

**`(reverse <item>) -> <item>`**

Returns a new list containing the items of `<item>` in reverse order or an atom containing the characters of `<item>` in reverse order.

**`(slice <item> <start: number> [end: number]) -> <item>`**

Returns a sub-list or sub-atom of `<item>` starting from the `<start>` index to the `<end>` index. If `<end>` is not provided, it slices to the end of the item. Negative indices can be used to count from the end.

**`(flatten <list> [depth: number]) -> <list>`**

Returns a new list with all sub-lists flattened into a single list. If a `depth` is provided, it specifies how many levels of nesting should be flattened.

**`(contains? <item> <subitem: item>) -> <true|false>`**

Returns `true` if the provided item is an atom that contains the subitem as a substring or a list that contains an item equal to the subitem, otherwise `false`.

**`(replace <item> <old: item> <new: item>) -> <item>`**

Returns a new atom or list with all occurrences of `<old>` replaced by `<new>`.

**`(unique <list>) -> <list>`**

Returns a new list containing only the unique items from the provided list, preserving their original order.

**`(chunk <list> <n: number>) -> <list>`**

Returns a new list of sub-lists, where each sub-list contains <n> items from the original list.

**`(find <list> <callable>) -> <item>`**

Returns the first item in the list for which the `<callable>` returns a truthy value. If no such item is found, returns `nil`.

**`(get-in <list> <path: list|atom> [default: item]) -> <item>`**

Returns the second item of the sub-list at the specified path within a nested association list. The path can be an atom for a single level or a list for nested access. If the path is not found, returns `[default]` or `nil`.

```lisp
(def data (("a" 1) ("b" 2) ("c" ("d" 3))))
(get-in data ("c" "d")) // Returns "3"
```

**`(assoc-in <list> <path: list|atom> <value: item>) -> <list>`**

Returns a new list with the value at the specified path replaced by `<value>`. If the path does not exist, it will be created.

**`(dissoc-in <list> <path: list|atom>) -> <list>`**

Returns a new list with the item at the specified path removed.

**`(update-in <list> <path: list|atom> <callable>) -> <list>`**

Returns a new list with the value at the specified path updated by applying `<callable>` to it.

**`(keys <list>) -> <list>`**

Returns a new list containing the first item of every sub-list.

**`(values <list>) -> <list>`**

Returns a new list containing the second item of every sub-list.

**`(merge <list> {list}) -> <list>`**

Returns a new list containing all unique keys from all provided association lists, with values from later lists overwriting those from earlier ones.

**`(explode <atom> {atom}) -> <list>`**

Returns the provided atoms as a list of the integer values for each character.

**`(implode <list> {list}) -> <atom>`**

Returns a new atom created by converting the provided lists of integer values back into their corresponding ASCII characters.

**`(repeat <item> <n: number>) -> <list>`**

Returns a new list containing the original item repeated `<n>` times.

---

#### String Manipulation

**`(starts-with? <item> <prefix: item>) -> <true|false>`**

Returns `true` if the provided item is an atom that starts with the prefix or a list whose first item is the prefix, otherwise `false`.

**`(ends-with? <item> <suffix: item>) -> <true|false>`**

Returns `true` if the provided item is an atom that ends with the suffix or a list whose last item is the suffix, otherwise `false`.

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

**`(atom? <item> {item}) -> <true|false>`**

Returns `true` if all items are atoms, otherwise `false`.

**`(int? <item> {item}) -> <true|false>`**

Returns `true` if all items are integer shaped atoms, otherwise `false`.

**`(float? <item> {item}) -> <true|false>`**

Returns `true` if all items are float shaped atoms, otherwise `false`.

**`(number? <item> {item}) -> <true|false>`**

Returns `true` if all items are integer or float shaped atoms, otherwise `false`.

**`(string? <item> {item}) -> <true|false>`**

Returns `true` if all items are string shaped (not number shaped) atoms, otherwise `false`.

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

**`(nil? <item> {item}) -> <true|false>`**

Returns `true` if all items are `nil`, otherwise `false`.

---

#### Type Casting

**`(int <atom>) -> <number>`**

Returns the integer shaped representation of the atom.

**`(float <atom>) -> <number>`**

Returns the float shaped representation of the atom.

**`(string <item>) -> <string>`**

Returns the stringified representation of the item.

---

#### System & Environment

**`(eval <item>) -> <item>`**

Evaluates the provided item and returns the result.

**`(parse <atom>) -> <expression>`**

Parses the provided string into a Reduct expression without evaluating it.

**`(run <atom>) -> <item>`**

Parses and evaluates the provided string.

**`(load! <path: string>) -> <item>`**

Parses and evaluates the file at the provided path, returning the result.

**`(read-file! <path: string>) -> <string>`**

Reads the file at the given path and returns its contents as a string atom without evaluating it.

**`(write-file! <path: string> <content: item>) -> <content: item>`**

Writes the string representation of `<content>` to the file at the given path.

**`(read-line!) -> <string>`**

Reads a single line of input from the standard input and returns it as an atom.

**`(read-char!) -> <string>`**

Reads a single character from the standard input and returns it as an atom.

**`(print! {item}) -> nil`**

Prints the string representation of all arguments to the standard output.

**`(println! {item}) -> nil`**

Prints the string representation of all arguments to the standard output, followed by a newline.

**`(ord <atom: string>) -> <number>`**

Converts an ascii character to its integer value.

**`(chr <number>) -> <atom>`**

Converts an integer value to its ascii character.

**`(format <format: string> {item}) -> <string>`**

Returns a formatted string using python-like formatting, where `{}` is used as a placeholder for the provided arguments.
Positional arguments can be used to specify the index of the argument to be used, for example `{0}`.

```lisp
(format "Hello, {}!" "World") // Returns "Hello, World!"
(format "{1} {0}" "World" "Hello") // Returns "Hello World"
```

**`(now!) -> <number>`**

Returns the current time in seconds since the Unix epoch.

**`(uptime!) -> <number>`**

Returns the current time in milliseconds since program start.

**`(env!) -> <list>`**

Returns all environment variables as association sub-lists.

**`(args!) -> <list>`**

Returns a list of all command line arguments.

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

**`(exp <val: number>) -> <number>`**

Returns the exponential of the argument.

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

**`(seed! <val: number>) -> <nil>`**

Seeds the random number generator.
