# Values and types

Every value in Ruva has a type, and `ruvac` checks all the types before your program runs. A mistake like adding text to a number is caught at compile time, not halfway through a run.

## The four basic types

| Type | Holds | Examples |
| --- | --- | --- |
| `int` | Whole numbers (signed 32-bit) | `0`, `42`, `-7` |
| `real` | Numbers with a fractional part (64-bit floating point) | `3.14`, `0.5`, `2.0` |
| `bool` | Truth values | `true`, `false` |
| `text` | A piece of text in UTF-8 | `"hello"`, `""` |

```ruva
launch() {
    keep count = 42;
    keep price = 9.75;
    keep ready = true;
    keep name = "Ruva";
    show(count, price, ready, name);
}
```

```text
42 9.75 true Ruva
```

## int

An `int` is a whole number from -2147483648 to 2147483647.

- Write it as plain decimal digits: `0`, `25`, `1000`.
- Leading zeros change nothing. `007` is 7 and `010` is 10. There is no octal.
- There are no hex numbers, no `_` separators and no exponents.
- A negative number is a minus sign in front of a number, so `-7` works as you'd expect. The smallest value, `-2147483648`, can be written directly.
- A literal outside the range is a compile error.

```ruva
launch() {
    keep big = 2147483647;
    keep small = -2147483648;
    keep padded = 007;
    show(big, small, padded);
}
```

```text
2147483647 -2147483648 7
```

Arithmetic that goes past the range stops the program with an error. [Operators](operators.md#checked-arithmetic) shows what happens.

## real

A `real` is a 64-bit floating-point number, the same as `double` in C.

- A real literal has digits on both sides of the point: `1.5`, `0.5`, `2.0`.
- `.5`, `5.` and `1e5` are not valid literals.
- `2` is an `int` and `2.0` is a `real`. They are different types.
- A literal too large to store is a compile error.

A `real` prints with up to 17 significant digits, with trailing zeros removed. Whole values print without a point. Many decimal fractions, such as 0.1, can't be stored exactly in binary, so the printed value shows the tiny difference:

```ruva
launch() {
    show(2.0, 2.50, 0.25, -1.75);
    show(3.14, 0.1, 0.000001);
    show(1234567890123456789.0);
}
```

```text
2 2.5 0.25 -1.75
3.1400000000000001 0.10000000000000001 9.9999999999999995e-07
1.2345678901234568e+18
```

Very large or very small values print with an exponent like `e+18`. Calculations that overflow print as `inf`, `-inf` or `nan` (see [Operators](operators.md#real-results)).

## bool

A `bool` is `true` or `false`. Comparisons such as `age >= 18` produce one.

Conditions in `check` and `during` must be a `bool`. Ruva never treats a number or text as true or false on its own.

## text

A `text` value is written in double quotes. It can hold any UTF-8 characters.

```ruva
launch() {
    show("héllo wörld ✓");
    show("Tab:\tend", "quote: \"hi\"", "back\\slash");
    show("line one\nline two");
}
```

```text
héllo wörld ✓
Tab:	end quote: "hi" back\slash
line one
line two
```

These are the only escapes:

| Escape | Means |
| --- | --- |
| `\n` | New line |
| `\t` | Tab |
| `\r` | Carriage return |
| `\\` | Backslash |
| `\"` | Double quote |

Rules for text:

- Single quotes are not allowed. `'hi'` is an error.
- A text literal must end on the line where it starts. Use `\n` for a line break.
- Any other escape, such as `\q`, is a compile error.
- `""` is the empty text.
- Text never changes in place. Joining with `+` makes new text (see [Operators](operators.md#joining-text)).
- `==` compares the characters, and case matters.

## Other types

| Type | What it is | Learn more |
| --- | --- | --- |
| `void` | No value at all. It is the result type of a function that gives nothing back. | [Functions](../flow/functions.md) |
| `file` | A handle to an open file, made by `fileOpen` | [Input and files](../programs/input-and-files.md) |
| `int[]`, `real[]`, `bool[]`, `text[]` | A resizable list of one basic type | [Arrays](../data/arrays.md) |
| Record types | Your own types with named fields, made with `object` | [Records](../data/records.md) |

## Converting between types

Ruva never changes a value's type on its own. When you need another type, call one of the four conversion functions.

| Function | Accepts | Gives |
| --- | --- | --- |
| `toInt` | `real` or `text` | `int`. A real loses its fraction, rounding toward zero. Text must be a whole number. |
| `toReal` | `int` or `text` | `real` |
| `toText` | `int`, `real` or `bool` | `text`, written the same way `show` prints it |
| `toBool` | `text` | `bool`. The text must be `true` or `false`, in lowercase. |

```ruva
launch() {
    show(toInt(3.99), toInt(-3.99), toInt(" 42 "));
    show(toReal(7), toReal("2.5"));
    show(toText(12) + "!", toText(true));
    show(toBool("true"), toBool(" false "));
}
```

```text
3 -3 42
7 2.5
12! true
true false
```

Spaces around the text are ignored. Passing a type a function doesn't accept is a compile error. Passing text that can't be converted, such as `toInt("12abc")`, stops the program with a runtime error. The [built-in functions](../reference/builtins.md) reference lists every detail.

## Common mistakes

An `int` is not a `real`, even when the number is whole:

```ruva
launch() {
    keep x: real = 5;
}
```

```text
main.ruv:2:20: error: initializer type does not match annotation
```

Write `5.0`, or convert with `toReal(5)`.

Text and numbers don't join directly:

```ruva
launch() {
    show("Total: " + 5);
}
```

```text
main.ruv:2:10: error: operator operands must have the same type
```

Write `"Total: " + toText(5)`, or pass both to `show`: `show("Total:", 5)`.

An escape that Ruva doesn't know:

```ruva
launch() {
    show("bad \q escape");
}
```

```text
main.ruv:2:15: error: invalid text escape
```

Next: [Variables](variables.md)
