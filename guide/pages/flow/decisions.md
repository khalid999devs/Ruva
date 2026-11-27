# Decisions

A decision runs some code only when a condition holds. Ruva has two forms: `check` for yes or no questions and `choose` for picking one case out of several.

## check

`check` runs its block when the condition is `true`.

```ruva
launch() {
    keep temperature = 31;
    check temperature > 30 {
        show("Hot day");
    }
    check temperature < 0 {
        show("Freezing");
    }
}
```

```text
Hot day
```

- The condition must be a `bool`. A number or text on its own is not a condition.
- The braces are required, even around a single statement.
- Parentheses around the condition are optional. `check (n > 1) { ... }` also works.
- No semicolon goes after the closing brace.

## check with otherwise

Add `otherwise` to run a different block when the condition is `false`. Exactly one of the two blocks runs.

```ruva
launch() {
    keep age = 15;
    check age >= 18 {
        show("Adult");
    } otherwise {
        show("Minor");
    }
}
```

```text
Minor
```

`otherwise` can sit on the same line as the closing brace or on the next line. Both are fine.

## Chains of checks

Ruva has no "else if". To test several conditions in a row, put another `check` inside the `otherwise` block:

```ruva
craft grade(mark: int) -> text {
    check mark >= 80 {
        give "A";
    } otherwise {
        check mark >= 50 {
            give "B";
        } otherwise {
            give "F";
        }
    }
}

launch() {
    show(grade(91), grade(64), grade(12));
}
```

```text
A B F
```

The conditions are tested from top to bottom, and the first one that is true wins.

## choose

`choose` compares one value against a list of cases and runs the block of the case that matches.

```ruva
craft dayName(day: int) -> text {
    choose day {
        when 1 => { give "Monday"; }
        when 2 => { give "Tuesday"; }
        when 3 => { give "Wednesday"; }
        otherwise => { give "Some other day"; }
    }
}

launch() {
    show(dayName(2), dayName(9));

    keep command = "stop";
    choose command {
        when "go" => {
            show("Moving");
        }
        when "stop" => {
            show("Stopping");
        }
    }
}
```

```text
Tuesday Some other day
Stopping
```

Each case is `when`, one literal, `=>` and a block. The optional `otherwise => { ... }` runs when no case matches.

## Rules for choose

- The value you choose on must be an `int` or a `text`. `real` and `bool` are not allowed.
- It is evaluated once, before any case is compared.
- Each `when` takes exactly one literal: a whole number without a sign, such as `0` or `42`, or a text such as `"go"`. Variables, expressions and negative numbers are not allowed.
- Every case literal must have the same type as the chosen value.
- Two cases can't have the same value. `1` and `01` count as the same value.
- `otherwise` is optional, and when present it must be the last case.
- Only the matching block runs. There is no fall-through to the next case and no `break`.
- If nothing matches and there is no `otherwise`, nothing happens and the program moves on.
- Each case needs braces around its block, even for one statement.

To branch on negative numbers or on ranges such as "50 to 79", use `check`.

## Decisions that give a value

Inside a function that returns a value, the compiler must be sure every path reaches a `give`.

- A `check` counts only when it has an `otherwise` and both blocks give.
- A `choose` counts only when it has an `otherwise` and every block gives.

The `grade` and `dayName` functions above both pass this test. [Functions](functions.md#every-path-must-give) explains the rule in full.

## Common mistakes

Using a number as a condition:

```ruva
launch() {
    keep n = 5;
    check n {
        show("yes");
    }
}
```

```text
main.ruv:3:11: error: condition must have type bool
```

Write `check n != 0 { ... }` instead.

Writing `otherwise check` without braces:

```ruva
launch() {
    keep n = 5;
    check n > 10 {
        show("big");
    } otherwise check n > 0 {
        show("small");
    }
}
```

```text
main.ruv:5:17: error: expected '{' for block
```

Put the inner `check` inside `otherwise { ... }`.

Using `if` and `else` from other languages:

```ruva
launch() {
    keep x = 3;
    if x > 1 {
        show("big");
    }
}
```

```text
main.ruv:3:5: error: expected an assignment or named call statement
```

A negative number in a `when`:

```ruva
launch() {
    keep n = -3;
    choose n {
        when -3 => { show("minus three"); }
    }
}
```

```text
main.ruv:4:14: error: when requires an unsigned integer or text literal
```

A case that appears twice:

```ruva
launch() {
    keep word = "b";
    choose word {
        when "a" => { show("a"); }
        when "a" => { show("again"); }
    }
}
```

```text
main.ruv:5:14: error: duplicate case literal
```

Next: [Loops](loops.md)
