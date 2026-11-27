# Operators

Operators combine values into new values: `+` adds, `<` compares, `&&` joins two conditions. Ruva checks every operator's types at compile time and checks integer arithmetic again while the program runs.

## Arithmetic

| Operator | Meaning | Works on |
| --- | --- | --- |
| `+` | Add | `int`, `real` |
| `-` | Subtract | `int`, `real` |
| `*` | Multiply | `int`, `real` |
| `/` | Divide | `int`, `real` |
| `%` | Remainder | `int` only |

```ruva
launch() {
    show(7 + 2, 7 - 2, 7 * 2, 7 / 2, 7 % 2);
    show(-7 / 2, -7 % 2, 7 % -2);
    show(7.0 / 2.0, 1.5 * 4.0, 0.1 + 0.2);
}
```

```text
9 5 14 3 1
-3 -1 1
3.5 6 0.30000000000000004
```

- Both sides must have the same type: `int` with `int` or `real` with `real`.
- `/` between two `int` values drops the fraction, rounding toward zero. `7 / 2` is 3 and `-7 / 2` is -3.
- The result of `%` has the sign of the left side. `-7 % 2` is -1.
- `real` arithmetic follows normal floating-point rules, so `0.1 + 0.2` is not exactly `0.3`.

## Mixing int and real

Ruva never mixes `int` and `real` for you. Convert one side first with `toReal` or `toInt`:

```ruva
launch() {
    keep x = 2;
    keep y = 3.5;
    show(toReal(x) * y, x * toInt(y));
}
```

```text
7 6
```

The first result is the `real` 7.0, printed as `7`. In the second, `toInt(3.5)` drops the fraction, so the result is the `int` 6.

## Comparison

| Operator | Meaning | Works on |
| --- | --- | --- |
| `<` `<=` `>` `>=` | Less, less or equal, greater, greater or equal | `int`, `real` |
| `==` `!=` | Equal, not equal | `int`, `real`, `bool`, `text` |

```ruva
launch() {
    show(3 < 5, 3 <= 3, 2.5 > 1.0, 4 >= 9);
    show(3 == 3, 3 != 3, "abc" == "abc", "abc" == "ABC", true == false);
}
```

```text
true true true false
true false true false false
```

- Both sides must have the same type, and the result is always a `bool`.
- Text equality compares the characters. Case matters.
- Text can't be ordered with `<` or `>`, and `bool` values can't either.
- Arrays and records can't be compared with `==`.

## Logical operators

| Operator | Meaning |
| --- | --- |
| `&&` | And: true when both sides are true |
| `||` | Or: true when at least one side is true |
| `!` | Not: flips true and false |

They only work on `bool` values. `&&` and `||` stop early: if the left side already decides the answer, the right side is never evaluated.

```ruva
craft yes(label: text) -> bool {
    show("checked", label);
    give true;
}

launch() {
    show(false && yes("right side"));
    show(true || yes("right side"));
    show(true && yes("right side"));
}
```

```text
false
true
checked right side
true
```

This lets you guard a risky check, as in `count > 0 && total / count > 10`.

## Joining text

`+` joins two text values into new text.

```ruva
launch() {
    keep first = "Ada";
    show("Hello, " + first + "!");
    show("Score: " + toText(42));
}
```

```text
Hello, Ada!
Score: 42
```

Both sides must be text. Use `toText` to turn a number or `bool` into text first.

## Unary operators

`-` in front of a number negates it. `!` in front of a `bool` flips it.

```ruva
launch() {
    keep x = 5;
    show(-x, -(x + 1), - -x, !(x > 3));
}
```

```text
-5 -6 5 false
```

There is no unary `+`. Writing `+5` is an error.

## Precedence and associativity

When an expression has several operators, higher rows in this table bind first. Operators in the same row are grouped from left to right.

| Level | Operators | Kind | Grouping |
| --- | --- | --- | --- |
| 1 (highest) | `f(...)` `a[i]` `r.field` | Call, index, field access | Left to right |
| 2 | `!` `-` | Unary not, negation | Right to left |
| 3 | `*` `/` `%` | Multiply, divide, remainder | Left to right |
| 4 | `+` `-` | Add, subtract, join text | Left to right |
| 5 | `<` `<=` `>` `>=` | Ordering | Left to right |
| 6 | `==` `!=` | Equality | Left to right |
| 7 | `&&` | And | Left to right |
| 8 (lowest) | `||` | Or | Left to right |

```ruva
launch() {
    show(2 + 3 * 4 - 1, (2 + 3) * (4 - 1));
    show(10 - 3 - 2, 100 / 10 / 2);
    show(1 + 2 < 4, true || false && false);
}
```

```text
13 15
5 5
true true
```

`10 - 3 - 2` means `(10 - 3) - 2`. `true || false && false` means `true || (false && false)`. Parentheses always win, so add them whenever an expression is hard to read.

Comparisons don't chain. `1 < 2 < 3` compares the `bool` from `1 < 2` with `3`, which is a type error. Write `1 < 2 && 2 < 3`.

## Evaluation order

Ruva evaluates from left to right. The left side of an operator runs before the right side, and function arguments run in the order you wrote them.

```ruva
craft loud(n: int) -> int {
    show("evaluating", n);
    give n;
}

launch() {
    show(loud(1) + loud(2) * loud(3));
}
```

```text
evaluating 1
evaluating 2
evaluating 3
7
```

Precedence decides how the values are grouped, not when each one runs. The only operators that can skip a side are `&&` and `||`.

## Checked arithmetic

An `int` result must fit between -2147483648 and 2147483647. Ruva checks every `int` operation while the program runs. When a check fails, the program prints an error and stops with exit status 1. Output printed before that point stays.

```ruva
launch() {
    keep x = 2147483647;
    show("before");
    show(x + 1);
    show("after");
}
```

```text
before
main.ruv:4:10: error: integer overflow
```

| What happens | Error message |
| --- | --- |
| An `int` result is out of range, for `+`, `-`, `*` or `/` | `integer overflow` |
| Negating -2147483648 | `integer overflow` |
| Dividing an `int` or a `real` by zero | `division by zero` |
| `%` with zero on the right | `remainder by zero` |

The check happens at runtime even when every value is a literal. `ruvac` accepts `show(2147483647 + 1);` and the program stops when it reaches that line.

## Real results

Apart from division by zero, `real` arithmetic doesn't stop the program. A result too large to store becomes `inf` or `-inf`, and an undefined one becomes `nan`.

```ruva
launch() {
    keep a = 10000000000.0;
    keep b = a * a * a * a;
    keep c = b * b * b * b;
    keep huge = c * c;
    show(c, huge, -huge, huge - huge);
}
```

```text
1e+160 inf -inf nan
```

Converting one of these with `toInt` is a runtime error.

## Common mistakes

Using `%` on `real` values:

```ruva
launch() {
    show(5.5 % 2.0);
}
```

```text
main.ruv:2:10: error: operator does not accept these operand types
```

Mixing `int` and `real`:

```ruva
launch() {
    show(1 + 2.0);
}
```

```text
main.ruv:2:10: error: operator operands must have the same type
```

Writing `=` where you mean `==`:

```ruva
launch() {
    keep x = 3;
    check x = 3 {
        show("three");
    }
}
```

```text
main.ruv:3:13: error: unexpected token in statement header
```

Next: [Decisions](../flow/decisions.md)
