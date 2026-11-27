# Errors

Ruva reports two kinds of errors. A compile error means `ruvac` found a problem in your source, so no program is built. A runtime error means the program was built but hit a problem while running, so it stops.

Both kinds look the same:

```text
main.ruv:3:5: error: expected ';' after declaration
```

That is the file, the line, the column, and a short message. The real output starts with the full path of the file, such as `/home/you/hello/main.ruv`. These pages shorten it to the file name.

## Compile errors

```ruva
launch() {
    keep total = 10
    show(total);
}
```

```text
main.ruv:3:5: error: expected ';' after declaration
```

The semicolon is missing at the end of line 2, but the error points at line 3, column 5. `ruvac` only notices the missing `;` when it reaches the next word, `show`. When an error makes no sense on its line, look at the end of the line above.

What to know about compile errors:

- `ruvac` stops at the first error it finds. Fix it and compile again to see the next one.
- `ruvac` exits with status 1, and no executable is written.
- `ruvac --check main.ruv` finds the same errors without building anything.

## Common compile errors

### Mixing int and real

Ruva never converts numbers for you. Both sides of an operator must have the same type.

```ruva
launch() {
    keep price = 2.5;
    keep count = 3;
    show(price * count);
}
```

```text
main.ruv:4:10: error: operator operands must have the same type
```

Convert one side with `toReal(count)` and it prints `7.5`. The same message appears for `"Line" + 2`, where `toText(2)` is the fix.

### Changing a keep

```ruva
launch() {
    keep count = 3;
    count = count + 1;
}
```

```text
main.ruv:3:5: error: binding is read-only
```

`keep` values never change. Declare the variable with `vary` if it needs to.

### A function that might not give a value

```ruva
craft sign(n: int) -> text {
    check n >= 0 {
        give "positive";
    }
}

launch() {
    show(sign(5));
}
```

```text
main.ruv:1:1: error: non-void function does not return on every path
```

When `n` is negative, nothing is given back. Add an `otherwise` branch that also uses `give`:

```ruva
craft sign(n: int) -> text {
    check n >= 0 {
        give "positive";
    } otherwise {
        give "negative";
    }
}

launch() {
    show(sign(5), sign(-2));
}
```

```text
positive negative
```

### More messages

| Message | Usual cause | Fix |
| --- | --- | --- |
| `expected ';' after declaration` | Missing `;`, reported at the start of the next line | Add the `;` on the line above |
| `unknown name` | A typo, a name used before its declaration, or used outside its block | Check the spelling and where it was declared |
| `unknown function` | Calling a function that doesn't exist | Check the spelling, or `bring` it from its module |
| `assignment type mismatch` | `name = 42;` when `name` holds text | Assign a value of the variable's type |
| `initializer type does not match annotation` | `keep age: int = "twenty";` | Make the value match the written type |
| `condition must have type bool` | `check count { ... }` with an `int` | Compare it, as in `count > 0` |
| `function argument count mismatch` | Too many or too few arguments | Match the function's parameters |
| `function argument type mismatch` | `add(1, "2")` for `int` parameters | Pass values of the right types |
| `void cannot initialize a value` | Storing the result of a function that returns nothing | Call it as a statement on its own |
| `void function cannot return a value` | `give 0;` inside `launch()` | Use `give;` or leave it out |
| `entry file requires launch()` | The file you compiled has no `launch()` | Add one, or compile the right file |
| `duplicate name in this scope` | Declaring the same name twice in one block | Pick a new name |
| `built-in names cannot be redefined or shadowed` | `keep show = 5;` | Built-in names are taken, pick another |
| `literal newline is not allowed in text` | A missing closing `"` | Close the text on the same line |
| `invalid text escape` | `"\q"` in text | Use `\n`, `\t`, `\r`, `\\` or `\"` |
| `integer literal is outside the signed 32-bit range` | A number like `3000000000` | Stay between -2147483648 and 2147483647 |
| `unknown source character` | A symbol Ruva doesn't use, like `@` | Remove it |

Errors about arrays, records, modules and files are listed on their own pages: [Arrays](../data/arrays.md), [Records](../data/records.md), [Modules](modules.md) and [Input and files](input-and-files.md).

## Runtime errors

A runtime error stops the program at once. Anything it printed before the error stays on the screen.

```ruva
launch() {
    show("start");
    show(2147483647 + 1);
    show("never printed");
}
```

```sh
ruvac main.ruv
./a.out
echo $?
```

```text
start
main.ruv:3:10: error: integer overflow
1
```

The line and column point at the expression that failed. The program exits with status 1. The error message goes to standard error, so you still see it when normal output is redirected to a file. If the failing code is in a module, the message names that module's file.

| Message | What went wrong |
| --- | --- |
| `integer overflow` | An `int` result left the range -2147483648 to 2147483647, as in `2147483647 + 1` |
| `division by zero` | Dividing by `0`, or by `0.0` with `real` numbers |
| `remainder by zero` | `%` with `0` on the right |
| `array index out of bounds` | An index below 0 or not less than the length |
| `cannot pop an empty array` | `pop` on an array with no items |
| `invalid integer text` | `ask(int)` or `toInt` got text that isn't a whole number |
| `integer text out of range` | The text was a whole number too big for `int` |
| `invalid real text` | `ask(real)` or `toReal` got text that isn't a number |
| `real text out of range` | The number in the text was too large, like `1e999` |
| `invalid boolean text` | `ask(bool)` or `toBool` got something other than `true` or `false` |
| `real value cannot convert to int` | `toInt` on a `real` outside the `int` range, like `toInt(10000000000.0)` |
| `unexpected end of input` | `ask` found no line left to read |
| `cannot open file` | A missing file, or a path that can't be opened |
| `invalid file mode` | `fileOpen` with a mode other than `"read"`, `"write"` or `"append"` |
| `file handle is not readable` / `not writable` | Reading or writing through a handle opened the other way |
| `file handle is closed` | Using a handle after `fileClose` |
| `invalid UTF-8 text` | A file whose bytes aren't valid text |
| `file close failed` | A handle left open couldn't be closed when the program ended. This is rare |

## Avoiding runtime errors

Most runtime errors can be prevented with a `check` before the risky step:

```ruva
launch() {
    show("Divide 100 by:");
    keep n = ask(int);
    check n == 0 {
        show("Can't divide by zero.");
    } otherwise {
        show(100 / n);
    }
}
```

```sh
printf '0\n' | ./a.out
```

```text
Divide 100 by:
Can't divide by zero.
```

The same idea works elsewhere. Compare an index with `length(...)` before using it, loop with `during length(stack) > 0` before calling `pop`, and use `fileExists` before `fileRead`.

## Exit codes

| Situation | Exit status |
| --- | --- |
| `ruvac` built the program | 0 |
| `ruvac` found a compile error | 1 |
| The program finished normally | 0 |
| The program stopped with a runtime error | 1 |

Next: [Built-in functions](../reference/builtins.md)
