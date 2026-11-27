# Program structure

Every Ruva program has the same shape: declarations at the top of the file and one `launch()` at the bottom. This page covers that shape and the small rules that hold it together.

## The entry point

`launch()` is where your program starts running.

```ruva
launch() {
    show("Hello from Ruva");
}
```

```text
Hello from Ruva
```

The statements inside the braces run from top to bottom. When the last one finishes, the program ends with exit status 0.

- A program has exactly one `launch()`.
- It takes no parameters, so the parentheses stay empty.
- It must be the last declaration in the file.
- It gives back no value. You can end it early with `give;` (see [Functions](../flow/functions.md)).

A program split over several files still has one `launch()`, in the file you compile. See [Modules](../programs/modules.md).

## Top level and statements

A Ruva file has two levels.

- The **top level** is everything outside braces. Only declarations go here: constants made with `keep`, functions made with `craft` and record types made with `object`. `bring` and `share`, which connect files into [modules](../programs/modules.md), belong here too.
- **Statements** are the instructions that run. They go inside `launch()` or inside a function.

```ruva
keep appName = "Counter";
keep pi = 3.14159;

craft circleArea(r: real) -> real {
    give pi * r * r;
}

launch() {
    show(appName);
    show(circleArea(2.0));
}
```

```text
Counter
12.56636
```

Top-level declarations can come in any order, as long as `bring` lines come first and `launch()` comes last. A function can call another function or use a constant that appears further down the file.

## Statements and semicolons

Simple statements end with a semicolon. Statements that carry their own block end at the closing brace and take no semicolon.

| Statement | Example | Ends with |
| --- | --- | --- |
| Declaration | `vary count = 0;` | `;` |
| Assignment | `count = count + 1;` | `;` |
| Function call | `show(count);` | `;` |
| Return | `give count;` | `;` |
| `check`, `choose`, `walk`, `during` | `check count > 0 { ... }` | `}` |

```ruva
launch() {
    vary count = 0;
    count = count + 1;
    show(count);
    check count > 0 {
        show("positive");
    }
}
```

```text
1
positive
```

Spaces and line breaks don't matter to the compiler. You can put two statements on one line or spread one statement over several lines.

A statement has to do something. An expression on its own, such as `3 + 4;`, is an error.

## Blocks

A block is a pair of braces with zero or more statements inside. The bodies of `launch()`, functions, `check` and loops are all blocks. You can also write a block on its own to keep a variable private to a few lines (see [scope](variables.md#scope)).

```ruva
launch() {
    keep total = 10;
    {
        keep half = total / 2;
        show(half);
    }
    show(total);
}
```

```text
5
10
```

An empty block `{}` is allowed.

## Comments

A comment starts with `//` and runs to the end of the line. The compiler ignores it.

```ruva
// Prints a greeting.
launch() {
    show("Hi"); // a comment can follow code
    show("a // inside text is not a comment");
}
```

```text
Hi
a // inside text is not a comment
```

Ruva has no block comments. Writing `/* ... */` is an error.

## Names

Variables, constants, functions, parameters and record types all need names. A name follows these rules:

- It starts with an ASCII letter or `_`.
- The rest are ASCII letters, digits or `_`.
- Case matters. `score`, `Score` and `SCORE` are three different names.
- It can't be one of the 25 [keywords](../reference/keywords.md).
- It can't be one of the 17 [built-in functions](../reference/builtins.md), such as `show` or `length`.

| Name | Allowed? |
| --- | --- |
| `total` | yes |
| `_count2` | yes |
| `firstName` | yes |
| `Keep` | yes, keywords are all lowercase |
| `2nd` | no, it starts with a digit |
| `walk` | no, it is a keyword |
| `length` | no, it is a built-in function |
| `café` | no, `é` is not ASCII |

## Printing with show

`show` prints its values on one line with a single space between them, then ends the line.

```ruva
launch() {
    show("Name:", "Ada", "Age:", 36);
    show();
    show("Price", 4.0, "Ratio", 0.25, "Done", true);
}
```

```text
Name: Ada Age: 36

Price 4 Ratio 0.25 Done true
```

- `show()` with no values prints an empty line.
- It accepts `int`, `real`, `bool` and `text`. Arrays, records and the result of a `void` function are rejected at compile time.
- Text prints as it is, without quotes.
- A `bool` prints as `true` or `false`.
- A `real` prints with up to 17 significant digits and no trailing zeros. `4.0` prints as `4`, and `0.1` prints as `0.10000000000000001`. [Values and types](values.md#real) explains why.
- All the values are worked out first, from left to right, and only then printed. If one of them fails, that `show` prints nothing.

```ruva
launch() {
    vary zero = 0;
    show("first line");
    show("this is never printed", 10 / zero);
}
```

```text
first line
main.ruv:4:35: error: division by zero
```

## Reading error messages

Compile errors and runtime errors share one format: the file, the line, the column and the message.

```text
main.ruv:4:35: error: division by zero
```

The real message starts with the full path of the file. These pages shorten it to the file name.

A compile error stops `ruvac` before any executable is made. A runtime error stops the running program with exit status 1. [Errors](../programs/errors.md) covers both in detail.

## Common mistakes

A missing semicolon is reported where the next token starts, which is often the following line:

```ruva
launch() {
    keep x = 1
    keep y = 2;
}
```

```text
main.ruv:3:5: error: expected ';' after declaration
```

A semicolon after a closing brace is an error, because blocks don't take one:

```ruva
launch() {
    check 1 < 2 {
        show("yes");
    };
}
```

```text
main.ruv:4:6: error: expected an expression
```

Statements can't sit at the top level:

```ruva
show("a");
launch() {
}
```

```text
main.ruv:1:1: error: expected a top-level keep, craft or object declaration
```

Nothing may follow `launch()`, not even a second `launch()`:

```ruva
launch() {
}
craft helper() -> void {}
```

```text
main.ruv:3:1: error: launch must be the last declaration
```

Next: [Values and types](values.md)
