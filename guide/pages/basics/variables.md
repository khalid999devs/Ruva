# Variables

A variable is a name that holds a value. Ruva has two kinds: `keep` for values that never change and `vary` for values that do.

## keep and vary

```ruva
launch() {
    keep name = "Mina";
    vary age = 20;
    age = age + 1;
    show(name, age);
}
```

```text
Mina 21
```

- `keep` makes a read-only variable. After it is set, it can't change.
- `vary` makes a variable you can change later with `=`.
- Every variable gets a value when you declare it. `vary total: int;` with no value is an error.

Use `keep` by default and switch to `vary` only when the value really needs to change. The compiler then catches accidental changes for you.

## Type annotations

The compiler works out a variable's type from its starting value. You can also write the type yourself after a colon:

```ruva
launch() {
    keep count: int = 3;
    keep price: real = 9.5;
    keep ready: bool = true;
    vary label: text = "box";
    label = label + "es";
    show(count, price, ready, label);
}
```

```text
3 9.5 true boxes
```

- The annotation must match the value exactly. `keep x: real = 5;` fails because `5` is an `int`.
- An annotation is needed for an empty array, such as `vary names: text[] = [];`. See [Arrays](../data/arrays.md).
- A variable's type never changes. Once `total` is an `int`, assigning text to it is an error.

## Changing a variable

Assignment uses `=` and ends with a semicolon. Only a `vary` variable can be assigned.

- Assignment is a statement, not a value. You can't write it inside a condition.
- There is no `+=` or `++`. Write `count = count + 1;`.

## Read-only rules

`keep` protects the whole value. For a record, no field can change. For an array, no element can change and nothing can be pushed or popped.

```ruva
object Point {
    x: int;
    y: int;
}

launch() {
    keep fixed = Point { x: 1, y: 2 };
    vary moving = fixed;
    moving.x = 10;
    show(fixed.x, moving.x);
}
```

```text
1 10
```

Assigning a record makes a full copy, so changing `moving` leaves `fixed` alone. Arrays can't be copied this way. See [Records](../data/records.md) and [Arrays](../data/arrays.md).

Two other kinds of names are read-only too: function parameters (see [Functions](../flow/functions.md)) and loop variables (see [Loops](../flow/loops.md)).

## Top-level constants

A `keep` written outside any function is a constant. Every function in the file can use it.

```ruva
keep limit = 100;
keep rate: real = 0.5;
keep title = "Report";
keep debug = false;
keep low = -40;

launch() {
    show(title, limit, rate, debug, low);
}
```

```text
Report 100 0.5 false -40
```

- Only `keep` is allowed at the top level. There are no global `vary` variables.
- The value must be a single literal: a number (with an optional minus sign), a text or `true`/`false`.
- Expressions, function calls and other names are not allowed. `keep area = 10 * 10;` fails.
- An annotation, if you write one, must be `int`, `real`, `bool` or `text`, and it must match exactly.
- Put `share` in front to let other files use the constant. See [Modules](../programs/modules.md).

## Scope

A variable exists from the line where it is declared to the end of the block that contains it. Outside that block, the name is unknown.

```ruva
launch() {
    {
        keep inside = 1;
    }
    show(inside);
}
```

```text
main.ruv:5:10: error: unknown name
```

Using a variable on a line above its declaration fails the same way.

## Shadowing

An inner block can declare a name that already exists outside it. The inner variable hides the outer one until the inner block ends.

```ruva
launch() {
    keep x = 1;
    {
        keep x = x + 10;
        show(x);
    }
    show(x);
}
```

```text
11
1
```

The starting value `x + 10` still sees the outer `x`, because the new `x` only exists once its declaration is done.

- Declaring the same name twice in the same block is an error.
- A variable inside a function can hide a top-level constant.
- A function's parameters belong to the same block as its body, so the body can't redeclare a parameter directly. A nested block inside the body can.
- The same goes for a loop variable and its loop body.
- Built-in function names such as `show` can never be reused.

## Common mistakes

Changing a `keep`:

```ruva
launch() {
    keep value = 1;
    value = 2;
}
```

```text
main.ruv:3:5: error: binding is read-only
```

Forgetting the starting value:

```ruva
launch() {
    vary total: int;
}
```

```text
main.ruv:2:20: error: expected '=' before initializer
```

Giving a variable a value of a different type:

```ruva
launch() {
    vary total = 0;
    total = "ten";
}
```

```text
main.ruv:3:13: error: assignment type mismatch
```

Declaring the same name twice in one block:

```ruva
launch() {
    keep x = 1;
    keep x = 2;
}
```

```text
main.ruv:3:10: error: duplicate name in this scope
```

Using `vary` at the top level:

```ruva
vary counter = 0;

launch() {
}
```

```text
main.ruv:1:1: error: expected a top-level keep, craft or object declaration
```

Next: [Operators](operators.md)
