# Arrays

An array is a list of values of the same type, kept in order. You can read any item by its position, change it, add items to the end and take items off the end.

```ruva
launch() {
    vary scores = [70, 85, 90];
    show(scores[0], scores[2]);
    show(length(scores));
}
```

```text
70 90
3
```

`[70, 85, 90]` is an array literal. `scores[0]` reads the first item and `length(scores)` tells you how many items there are.

## Array types

An array type is an element type followed by `[]`. There are four:

| Type | Holds | Example literal |
| --- | --- | --- |
| `int[]` | whole numbers | `[1, 2, 3]` |
| `real[]` | decimal numbers | `[2.5, 4.75]` |
| `bool[]` | `true` or `false` | `[true, false]` |
| `text[]` | text | `["Asha", "Rafi"]` |

The items are always one of the four primitive types. You can't make an array of records, an array of arrays or an array of files.

```ruva
launch() {
    keep prices = [2.5, 4.75, 10.25];
    keep flags = [true, false, true];
    keep names = ["Asha", "Rafi"];
    vary empty: int[] = [];
    show(prices[1], flags[0], names[1], length(empty));
}
```

```text
4.75 true Rafi 0
```

## Array literals and empty arrays

Write the items between square brackets, separated by commas. Ruva works out the array type from the items, so they must all have the same type. `[1, 2.5]` is an error.

An empty literal `[]` has no items to look at, so Ruva can't tell what type it is. Give the variable a type, as `vary empty: int[] = [];` does above. Without one you get `empty array needs an expected array type`.

## Reading items

Positions start at 0. An array with three items has positions 0, 1 and 2. The position, called the index, must be an `int`.

Every index is checked while the program runs. If it is outside the array, the program stops with an error and exit code 1:

```ruva
launch() {
    vary scores = [70, 85, 90];
    show(scores[3]);
}
```

```text
main.ruv:3:10: error: array index out of bounds
```

A negative index fails the same way.

## Adding and removing items

`push(array, value)` adds a value to the end. `pop(array)` removes the last item and gives it back. `length(array)` gives the number of items as an `int`.

```ruva
launch() {
    vary names: text[] = [];
    push(names, "Asha");
    push(names, "Rafi");
    push(names, "Mina");
    show(length(names));

    keep last = pop(names);
    show(last, length(names));
}
```

```text
3
Mina 2
```

The rules:

- `push` and `pop` only work on an array stored in a `vary` variable.
- The value you push must match the element type. You can't push `1` into a `real[]`, write `1.0`.
- `push` gives nothing back, so you can't store its result.
- `pop` on an empty array stops the program.

```ruva
launch() {
    vary stack: int[] = [];
    push(stack, 1);
    show(pop(stack));
    show(pop(stack));
}
```

```text
1
main.ruv:5:10: error: cannot pop an empty array
```

## Changing items

Assign to an index to change one item. The array must be `vary`.

```ruva
launch() {
    vary scores = [70, 85, 90];
    scores[1] = 88;
    scores[0] = scores[0] + 5;
    show(scores[0], scores[1], scores[2]);
}
```

```text
75 88 90
```

Writing to an index that doesn't exist is the same runtime error as reading one.

## Replacing a whole array

A `vary` array can take a new array literal. The new literal may use the old items, because Ruva builds the new array before it swaps it in. An empty `[]` works here too, since the variable already has a type.

```ruva
launch() {
    vary scores = [70, 85, 90];
    scores = [length(scores), scores[0] + 1];
    show(scores[0], scores[1]);

    scores = [];
    show(length(scores));
}
```

```text
3 71
0
```

## Walking an array

`walk item in array` runs its body once for each item, from first to last. To get positions as well, walk over a range of indexes.

```ruva
launch() {
    keep scores = [70, 85, 90];
    vary total = 0;
    walk score in scores {
        total = total + score;
    }
    show("total:", total);

    walk i from 0 until length(scores) {
        show(i, scores[i]);
    }
}
```

```text
total: 245
0 70
1 85
2 90
```

While you walk an array with `walk ... in`:

- The loop variable is a read-only copy of the current item.
- You can't `push`, `pop` or replace the array you are walking.
- You can change its items, and later steps see the new values.

```ruva
launch() {
    vary scores = [70, 85, 90];
    walk score in scores {
        scores[2] = 100;
        show(score);
    }
}
```

```text
70
85
100
```

Filling a different array during the walk is fine:

```ruva
launch() {
    keep scores = [70, 45, 90, 30];
    vary passed: int[] = [];
    walk score in scores {
        check score >= 50 {
            push(passed, score);
        }
    }
    show(length(passed), passed[0], passed[1]);
}
```

```text
2 70 90
```

## Arrays stay where they are made

An array belongs to the one variable that holds it. Ruva never copies or shares a whole array, so:

- You can't copy an array into another variable. `keep b = a;` fails with `array initialization and replacement require a fresh literal`.
- Functions can't take or return arrays.
- Arrays can't be top-level constants or record fields.
- `show` can't print a whole array, and `==` can't compare two arrays.

When you need a copy, build a new literal from the items. When a function needs the data, pass the items:

```ruva
craft average(a: int, b: int, c: int) -> real {
    give toReal(a + b + c) / 3.0;
}

launch() {
    keep scores = [70, 80, 90];
    vary copy: int[] = [scores[0], scores[1], scores[2]];
    push(copy, 100);
    show(length(scores), length(copy));
    show(average(scores[0], scores[1], scores[2]));
}
```

```text
3 4
80
```

## Common mistakes

| Message | Usual cause | Fix |
| --- | --- | --- |
| `array initialization and replacement require a fresh literal` | Copying, as in `keep b = a;` | Build a new literal from the items |
| `empty array needs an expected array type` | `vary items = [];` | Add a type: `vary items: int[] = [];` |
| `array element type mismatch` | Mixed items, like `[1, 2.5]` | Use one type for every item |
| `binding is read-only` | `push`, `pop` or an index change on a `keep` array | Declare the array with `vary` |
| `push element type mismatch` | Pushing a value of the wrong type | Convert the value first |
| `array type is not allowed here` | `int[]` as a parameter, result or record field | Pass or store the items instead |
| `cannot resize or replace an active array walk target` | `push`, `pop` or replacement inside a walk over the same array | Collect changes in another array |
| `indexing requires an array and an int index` | `scores[1.0]` | Use an `int` index |
| `array elements must be primitive values` | A record inside an array literal | Keep records in separate variables |
| `show accepts only primitive values` | `show(scores)` | Walk the array and show each item |

The two runtime errors, `array index out of bounds` and `cannot pop an empty array`, are covered with the others in [Errors](../programs/errors.md).

Next: [Records](records.md)
