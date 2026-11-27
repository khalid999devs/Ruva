# Loops

A loop runs the same block more than once. Ruva has `during`, which repeats while a condition holds, and `walk`, which steps through a range of numbers, an array or the fields of a record.

## during

`during` checks its condition, runs the block if it is `true`, and repeats.

```ruva
launch() {
    vary count = 3;
    during count > 0 {
        show("count is", count);
        count = count - 1;
    }
    show("liftoff");
}
```

```text
count is 3
count is 2
count is 1
liftoff
```

- The condition must be a `bool`.
- It is checked again before every pass. If it is `false` at the start, the block never runs.
- Something in the block has to change the condition, or the loop never ends.

## walk from until

`walk i from start until end` counts upward through a range of `int` values.

```ruva
launch() {
    walk i from 0 until 3 {
        show("i =", i);
    }
}
```

```text
i = 0
i = 1
i = 2
```

The start is included and the end is **excluded**. `from 0 until 3` gives 0, 1 and 2, which is three passes. This matches how arrays are indexed, so `walk i from 0 until length(scores)` visits every index.

- Both bounds must be `int`.
- The step is always +1. There is no way to count down or skip values directly.
- If the start is equal to or greater than the end, the block never runs.
- Both bounds are evaluated once, before the first pass. Changing a variable used in the bound doesn't change how many times the loop runs.

To count down, compute the value from the loop variable:

```ruva
launch() {
    walk i from 0 until 3 {
        show(3 - i);
    }
}
```

```text
3
2
1
```

## walk in an array

`walk x in values` visits each element of an array, in order from the first to the last.

```ruva
launch() {
    keep names = ["Ada", "Linus", "Grace"];
    walk name in names {
        show("Hello,", name);
    }

    vary total = 0;
    walk n in [4, 5, 6] {
        total = total + n;
    }
    show(total);
}
```

```text
Hello, Ada
Hello, Linus
Hello, Grace
15
```

- The loop variable holds a copy of the element. Changing it is not allowed.
- While the walk is running, you can't `push` to, `pop` from or replace the array you are walking.
- You can change an element with `values[i] = ...` if the array is `vary`. A later pass sees the new value.

[Arrays](../data/arrays.md) covers arrays in detail.

## walk over record fields

`walk` can also visit the fields of a record, in the order they were declared. Each field gives you `field.key`, the field name, and `field.value`, the value written as text.

```ruva
object Pet {
    name: text;
    age: int;
}

launch() {
    keep pet = Pet { name: "Milo", age: 3 };
    walk field in pet {
        show(field.key, "=", field.value);
    }
}
```

```text
name = Milo
age = 3
```

See [Records](../data/records.md#walking-the-fields) for the details.

## Loop variable rules

- Each `walk` makes a new loop variable. It exists only inside the loop block.
- The loop variable is read-only. Assigning to it is an error.
- It can have the same name as a variable outside the loop. The outer one is hidden until the loop ends.
- It belongs to the same block as the loop body, so the body can't declare another variable with that name directly.

```ruva
launch() {
    keep i = 99;
    walk i from 0 until 2 {
        show(i);
    }
    show(i);
}
```

```text
0
1
99
```

## Nested loops

A loop can sit inside another loop. The inner loop runs completely on every pass of the outer one.

```ruva
launch() {
    walk row from 1 until 3 {
        walk col from 1 until 4 {
            show(row, "x", col, "=", row * col);
        }
    }
}
```

```text
1 x 1 = 1
1 x 2 = 2
1 x 3 = 3
2 x 1 = 2
2 x 2 = 4
2 x 3 = 6
```

## Stopping early

Ruva has no `break` or `continue`. To leave a loop early, put it in a function and use `give`. A `give` ends the whole function at once, from any depth of loops.

```ruva
craft firstOver(limit: int) -> int {
    keep values = [3, 8, 12, 5, 20];
    walk v in values {
        check v > limit {
            give v;
        }
    }
    give -1;
}

launch() {
    show(firstOver(10), firstOver(50));
}
```

```text
12 -1
```

With `during`, you can also add a `bool` variable to the condition and set it to `false` when you want the loop to end.

## Common mistakes

Changing the loop variable:

```ruva
launch() {
    walk i from 0 until 3 {
        i = 10;
    }
}
```

```text
main.ruv:3:9: error: binding is read-only
```

Growing the array you are walking:

```ruva
launch() {
    vary items = [1, 2];
    walk x in items {
        push(items, x);
    }
}
```

```text
main.ruv:4:14: error: cannot resize or replace an active array walk target
```

Using `break`:

```ruva
launch() {
    walk i from 0 until 10 {
        break;
    }
}
```

```text
main.ruv:3:9: error: expected an assignment or named call statement
```

Next: [Functions](functions.md)
