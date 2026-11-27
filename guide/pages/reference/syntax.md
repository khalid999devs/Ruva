# Syntax

This page collects all of Ruva's syntax in one place. It is meant for looking things up. The pages linked from each section explain the rules with examples.

In the patterns below, `Name` stands for any name you choose, `expr` for any expression, `type` for a type and `{ ... }` for a block of statements. A trailing `...` means "more of the same, separated by commas".

## Tokens

| Token | Form | Examples |
| --- | --- | --- |
| Name | An ASCII letter or `_`, then letters, digits or `_`. Case matters. | `total`, `_tmp`, `Point2` |
| Integer | Decimal digits only. Leading zeros don't make it octal. | `0`, `42`, `007` |
| Real | Digits, a point, digits | `3.14`, `0.5`, `2.0` |
| Text | Double quotes on one line. Escapes: `\n` `\t` `\r` `\\` `\"` | `"hi"`, `"a\tb"` |
| Bool | `true` or `false` | |
| Comment | `//` to the end of the line | `// note` |
| Operators | `+ - * / % == != < <= > >= && || ! =` | |
| Punctuation | `( ) { } [ ] , ; : . -> =>` | |

Spaces, tabs and line breaks separate tokens and are otherwise ignored. There are 25 [keywords](keywords.md) and 17 protected [built-in function](builtins.md) names.

## File layout

The parts of a file always come in this order:

1. `bring` lines, if any
2. Top-level declarations, in any order
3. `launch() { ... }`, only in the file you compile, and always last

A module file brought in by another file has steps 1 and 2 only. See [Program structure](../basics/program.md) and [Modules](../programs/modules.md).

## Top-level declarations

| Pattern | Example |
| --- | --- |
| `bring { Name, ... } from "path.ruv";` | `bring { Item } from "model/item.ruv";` |
| `keep Name = literal;` | `keep limit = 100;` |
| `keep Name: type = literal;` | `keep rate: real = 0.5;` |
| `craft Name(param: type, ...) -> type { ... }` | `craft add(a: int, b: int) -> int { give a + b; }` |
| `object Name { field: type; ... }` | `object Point { x: int; y: int; }` |
| `share` before `keep`, `craft` or `object` | `share craft add(a: int, b: int) -> int { ... }` |
| `launch() { ... }` | `launch() { show("hi"); }` |

A top-level `literal` is one number (with an optional `-`), one text, `true` or `false`. In `object`, each field line ends with `;`.

## Statements

| Pattern | Example | Page |
| --- | --- | --- |
| `keep Name = expr;` | `keep total = 0;` | [Variables](../basics/variables.md) |
| `vary Name: type = expr;` | `vary names: text[] = [];` | [Variables](../basics/variables.md) |
| `Name = expr;` | `total = total + 1;` | [Variables](../basics/variables.md) |
| `Name.field = expr;` | `p.x = 5;` | [Records](../data/records.md) |
| `Name[expr] = expr;` | `scores[0] = 90;` | [Arrays](../data/arrays.md) |
| `call(...);` | `show(total);` | [Functions](../flow/functions.md) |
| `give expr;` or `give;` | `give a + b;` | [Functions](../flow/functions.md#give) |
| `check expr { ... }` | `check n > 0 { ... }` | [Decisions](../flow/decisions.md) |
| `check expr { ... } otherwise { ... }` | | [Decisions](../flow/decisions.md) |
| `choose expr { when literal => { ... } ... otherwise => { ... } }` | | [Decisions](../flow/decisions.md#choose) |
| `walk Name from expr until expr { ... }` | `walk i from 0 until 3 { ... }` | [Loops](../flow/loops.md) |
| `walk Name in expr { ... }` | `walk name in names { ... }` | [Loops](../flow/loops.md) |
| `during expr { ... }` | `during count > 0 { ... }` | [Loops](../flow/loops.md#during) |
| `{ ... }` | A block on its own | [Program structure](../basics/program.md#blocks) |

Statements in the upper part of the table end with `;`. Statements that end in a block take no `;`. In `choose`, each `when` takes an unsigned integer or a text literal, and `otherwise =>` is optional but must come last.

## Expressions

| Pattern | Example |
| --- | --- |
| Literal | `42`, `2.5`, `"hi"`, `true` |
| Name | `total` |
| Parentheses | `(a + b) * 2` |
| Unary operator | `-x`, `!done` |
| Binary operator | `a + b`, `x >= 10`, `ok && ready` |
| Function call | `add(1, 2)` |
| Field access | `p.x`, `field.key` |
| Index | `scores[0]` |
| Array literal | `[1, 2, 3]`, `[]` |
| Record literal | `Point { x: 1, y: 2 }`, `Empty {}` |
| Typed input | `ask(int)`, `ask(text)` |

Arguments, array items and record fields are separated by commas, with no comma after the last one.

## Types

| Type | Local variable | Top-level constant | Parameter | Return type | Record field | Array item |
| --- | --- | --- | --- | --- | --- | --- |
| `int`, `real`, `bool`, `text` | Yes | Yes | Yes | Yes | Yes | Yes |
| `int[]`, `real[]`, `bool[]`, `text[]` | Yes | No | No | No | No | No |
| `file` | Yes | No | No | No | No | No |
| A record type | Yes | No | Yes | Yes | No | No |
| `void` | No | No | No | Yes | No | No |

## Operator precedence

From the highest (binds first) to the lowest. Operators in the same row are grouped from left to right, except the unary row, which groups from right to left.

| Level | Operators | Meaning | Operand types |
| --- | --- | --- | --- |
| 1 | `f(...)` `a[i]` `r.field` | Call, index, field access | |
| 2 | `!` `-` (prefix) | Not, negation | `bool` for `!`, `int` or `real` for `-` |
| 3 | `*` `/` `%` | Multiply, divide, remainder | Same numeric type. `%` is `int` only. |
| 4 | `+` `-` | Add, subtract. `+` also joins text. | Same numeric type, or two `text` |
| 5 | `<` `<=` `>` `>=` | Ordering | Same numeric type |
| 6 | `==` `!=` | Equality | Same basic type |
| 7 | `&&` | And, stops early on `false` | `bool` |
| 8 | `||` | Or, stops early on `true` | `bool` |

Operands are evaluated from left to right. Assignment with `=` is a statement, not an operator, so it never appears inside an expression. See [Operators](../basics/operators.md) for examples.

See also: [Keywords](keywords.md) and [Built-in functions](builtins.md)
