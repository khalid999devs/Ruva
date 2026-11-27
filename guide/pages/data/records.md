# Records

A record keeps a few named values together as one value. You describe its shape once with `object`, then build as many records of that shape as you like.

```ruva
object Student {
    name: text;
    marks: int;
    passed: bool;
}

launch() {
    vary student = Student { name: "Rafi", marks: 85, passed: true };
    show(student.name, student.marks, student.passed);

    student.marks = 91;
    show(student.marks);
}
```

```text
Rafi 85 true
91
```

Each named value inside a record is a field. `student.name` reads a field and `student.marks = 91;` changes one.

## Declaring an object

An `object` declaration lists the fields and their types:

```ruva
object Point {
    x: int;
    y: int;
}
```

The rules:

- Write `object` declarations at the top level of a file, outside `launch()` and functions.
- Every field is `name: type;` and ends with a semicolon.
- Field types are `int`, `real`, `bool` or `text`. A field can't hold an array, a file or another record.
- Field names in one object must be different.
- Records have data only. There are no methods.

## Building a record

A record literal is the object name followed by every field and its value in braces, separated by commas. You must give each field exactly once, in any order.

```ruva
object Point {
    x: int;
    y: int;
}

launch() {
    keep a = Point { y: 2, x: 1 };
    vary b: Point = Point { x: 3, y: 4 };
    show(a.x, a.y, b.x, b.y);
}
```

```text
1 2 3 4
```

The type of `a` comes from the literal. You can also write it, as with `b: Point`.

## Empty records

An object may have no fields at all. Its literal is the name followed by `{}`.

```ruva
object Empty {}

launch() {
    keep nothing = Empty {};
    walk field in nothing {
        show("never printed");
    }
    show("done");
}
```

```text
done
```

## Reading and changing fields

Use a dot to reach a field. Changing a field needs a `vary` record. A `keep` record can't change in any way, field by field included.

```ruva
object Student {
    name: text;
    marks: int;
    passed: bool;
}

launch() {
    keep student = Student { passed: true, name: "Rafi", marks: 85 };
    student.marks = 91;
}
```

```text
main.ruv:9:5: error: binding is read-only
```

## Records are copied

Assigning a record to another variable makes a full copy. Changing the copy leaves the original alone.

```ruva
object Point {
    x: int;
    y: int;
}

launch() {
    keep a = Point { x: 1, y: 2 };
    vary b = a;
    b.x = 10;
    show(a.x, b.x);
}
```

```text
1 10
```

A `vary` record can also take a whole new record of the same type, for example `b = Point { x: 0, y: 0 };`.

## Passing and returning records

Functions can take records as parameters and give records back. The function gets its own copy. Parameters are read-only, so to change one, copy it into a `vary` variable first.

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
    keep next = moved(start, 5);
    show(start.x, next.x);
}
```

```text
1 6
```

Writing `p.x = 0;` inside the function gives `binding is read-only`.

## Walking the fields

`walk field in record` visits every field in the order the object declares them. Inside the loop, `field.key` is the field name and `field.value` is its value. Both are `text`, so numbers and booleans arrive already converted.

```ruva
object Student {
    name: text;
    marks: int;
    average: real;
    passed: bool;
}

launch() {
    keep student = Student { name: "Rafi", marks: 85, average: 82.5, passed: true };
    walk field in student {
        show(field.key, "=", field.value);
    }
}
```

```text
name = Rafi
marks = 85
average = 82.5
passed = true
```

Because both are text, you can join them with `+` into a longer text, such as a report to save in a file. The name `field` is your choice. `walk f in student` works the same way with `f.key` and `f.value`.

### The walk uses a snapshot

Before the first step, Ruva takes a snapshot of all the fields as text. If the body changes the record, the walk still shows the values from the start. The record itself does change.

```ruva
object Counter {
    label: text;
    count: int;
}

launch() {
    vary counter = Counter { label: "clicks", count: 1 };
    walk field in counter {
        counter.count = counter.count + 10;
        show(field.key, field.value);
    }
    show("after:", counter.count);
}
```

```text
label clicks
count 1
after: 21
```

The walk variable is not a record. You can only read `field.key` and `field.value`. You can't assign to them, store `field` in a variable or ask for any other part of it.

## Common mistakes

Leaving out a field is the usual one:

```ruva
object Point {
    x: int;
    y: int;
}

launch() {
    keep p = Point { x: 1 };
}
```

```text
main.ruv:7:14: error: record constructor must initialize every field
```

| Message | Usual cause | Fix |
| --- | --- | --- |
| `record constructor must initialize every field` | A field is missing from the literal | Give every field a value |
| `unknown record field` | A misspelled or extra field name | Check the object declaration |
| `duplicate constructor field` | The same field twice in a literal | Remove one |
| `record field initializer type mismatch` | `y: 2.5` for an `int` field | Use a value of the field's type |
| `binding is read-only` | Changing a field of a `keep` record or a parameter | Use `vary`, or copy into a `vary` first |
| `type is not allowed here` | A record or file as a field type | Use primitive fields only |
| `record walk binding exposes only key and value` | `field.name` inside a field walk | Use `field.key` or `field.value` |
| `show accepts only primitive values` | `show(student)` | Show the fields, or walk them |
| `operator does not accept these operand types` | `a == b` on two records | Compare the fields one by one |
| `assignment type mismatch` | Assigning a record of a different object type | Two objects are different types even with the same fields |
| `expected a constant literal` | A record as a top-level `keep` | Build records inside `launch()` or a function |

Next: [Modules](../programs/modules.md)
