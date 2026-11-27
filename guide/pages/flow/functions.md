# Functions

A function is a named piece of code you can run as often as you like. You define one with `craft` and send a value back with `give`.

## Defining and calling a function

```ruva
craft add(a: int, b: int) -> int {
    give a + b;
}

craft greet(name: text) -> void {
    show("Hello,", name);
}

launch() {
    keep sum = add(2, 3);
    show(sum);
    greet("Sam");
}
```

```text
5
Hello, Sam
```

A function definition has these parts, in order:

| Part | In `add` |
| --- | --- |
| The keyword `craft` | `craft` |
| The name | `add` |
| Parameters in parentheses | `(a: int, b: int)` |
| `->` and the return type | `-> int` |
| The body, a block | `{ give a + b; }` |

The return type is always required. A function that gives nothing back uses `-> void`.

## Parameters

Each parameter is a name, a colon and a type. Separate them with commas. A function may also have no parameters at all: `craft now() -> int`.

- A parameter can be `int`, `real`, `bool`, `text` or a record type.
- Arrays, files and `void` can't be parameters.
- Parameters are read-only inside the function. Copy one into a `vary` variable if you need to change it.
- Every call must pass the right number of arguments, each with the right type.
- Arguments are passed as copies. A record passed to a function is copied, so the function can't change the caller's record.

```ruva
object Point {
    x: int;
    y: int;
}

craft moved(p: Point, dx: int) -> Point {
    vary copy = p;
    copy.x = copy.x + dx;
    give copy;
}

launch() {
    keep start = Point { x: 1, y: 2 };
    keep next = moved(start, 10);
    show(start.x, next.x);
}
```

```text
1 11
```

## give

`give value;` ends the function and hands `value` back to the caller. The value must match the return type exactly.

In a `void` function, `give;` with no value ends the function early. It is optional at the end, because a `void` function also ends when it runs out of statements. In `launch()`, `give;` ends the program.

`give` leaves the whole function at once, even from inside a loop or a `check`. [Loops](loops.md#stopping-early) uses this to stop a loop early.

## void functions

A `void` function does something but gives no value back. You call it as a statement on its own line.

```ruva
craft countdown(n: int) -> void {
    check n == 0 {
        show("go");
        give;
    }
    show(n);
    countdown(n - 1);
}

launch() {
    countdown(3);
}
```

```text
3
2
1
go
```

There is nothing to store or print, so `keep x = greet("a");` and `show(greet("a"));` are both errors. The first one reports `void cannot initialize a value`.

You can also call a function that does return a value as a statement on its own. The value is thrown away.

## Every path must give

A function with a return type other than `void` must reach a `give` on every possible path. The compiler checks this before the program runs.

- A `give` as the last statement of the body always counts.
- A `check` counts only if it has an `otherwise` and both blocks end in a `give`.
- A `choose` counts only if it has an `otherwise` and every block ends in a `give`.
- Loops never count, even `during true`, because the compiler doesn't look at how many times a loop runs.

```ruva
craft sign(n: int) -> int {
    check n > 0 {
        give 1;
    }
    check n < 0 {
        give -1;
    }
}

launch() {
    show(sign(3));
}
```

```text
main.ruv:1:1: error: non-void function does not return on every path
```

Here a zero falls through both checks. Ending with `give 0;` fixes it.

## Calling order

Functions are all declared at the top level, and every function can call every other one, no matter which comes first in the file.

```ruva
craft first(n: int) -> int {
    give later(n) + 1;
}

craft later(n: int) -> int {
    give n * 2;
}

launch() {
    show(first(4));
}
```

```text
9
```

- Functions can't be defined inside other functions or inside `launch()`.
- Each function name must be unique in its file. Two functions can't share a name, even with different parameters.
- Arguments are evaluated from left to right before the function runs.

## Recursion

A function may call itself. Each call gets its own parameters and variables.

```ruva
craft fib(n: int) -> int {
    check n < 2 {
        give n;
    }
    give fib(n - 1) + fib(n - 2);
}

launch() {
    show(fib(1), fib(5), fib(10), fib(20));
}
```

```text
1 5 55 6765
```

Make sure every recursive function has a case that stops calling itself, like `n < 2` above. A function that calls itself without end makes the program crash.

## What can go in and come out

| Type | As a parameter | As a return type |
| --- | --- | --- |
| `int`, `real`, `bool`, `text` | Yes | Yes |
| A record type | Yes, as a copy | Yes, as a copy |
| `void` | No | Yes, meaning no value |
| Arrays | No | No |
| `file` | No | No |

Arrays and files stay in the function that made them. See [Arrays](../data/arrays.md) and [Input and files](../programs/input-and-files.md).

## Common mistakes

Leaving out the return type:

```ruva
craft hello() {
    show("hi");
}

launch() {
    hello();
}
```

```text
main.ruv:1:15: error: expected '->' before return type
```

Changing a parameter:

```ruva
craft bump(n: int) -> int {
    n = n + 1;
    give n;
}

launch() {
    show(bump(1));
}
```

```text
main.ruv:2:5: error: binding is read-only
```

Giving back the wrong type:

```ruva
craft half(n: int) -> int {
    give 2.5;
}

launch() {
    show(half(3));
}
```

```text
main.ruv:2:10: error: return value type mismatch
```

Passing the wrong number of arguments:

```ruva
craft add(a: int, b: int) -> int {
    give a + b;
}

launch() {
    show(add(1));
}
```

```text
main.ruv:6:10: error: function argument count mismatch
```

Next: [Arrays](../data/arrays.md)
