# Built-in functions

Ruva has 17 built-in functions. They are always available, with no `bring` needed. Their names are protected, so you can't use them for your own variables or functions.

## Summary

| Name | Signature | What it does |
| --- | --- | --- |
| [`ask`](#ask) | `ask(type) -> type` | Reads one line of input as `int`, `real`, `bool` or `text` |
| [`show`](#show) | `show(value, ...) -> void` | Prints values on one line |
| [`toInt`](#toint) | `toInt(real or text) -> int` | Converts to a whole number |
| [`toReal`](#toreal) | `toReal(int or text) -> real` | Converts to a decimal number |
| [`toText`](#totext) | `toText(int, real or bool) -> text` | Converts to text |
| [`toBool`](#tobool) | `toBool(text) -> bool` | Converts `"true"` or `"false"` to a `bool` |
| [`push`](#push) | `push(array, value) -> void` | Adds a value to the end of an array |
| [`pop`](#pop) | `pop(array) -> element` | Removes and gives back the last item |
| [`length`](#length) | `length(array) -> int` | Counts the items in an array |
| [`fileWrite`](#filewrite) | `fileWrite(path, content) -> bool` | Replaces a file's content |
| [`fileAppend`](#fileappend) | `fileAppend(path, content) -> bool` | Adds to the end of a file |
| [`fileRead`](#fileread) | `fileRead(path) -> text` | Reads a whole file |
| [`fileExists`](#fileexists) | `fileExists(path) -> bool` | Checks that a file exists |
| [`fileOpen`](#fileopen) | `fileOpen(path, mode) -> file` | Opens a file and gives back a handle |
| [`fileReadFrom`](#filereadfrom) | `fileReadFrom(handle) -> text` | Reads the rest of an open file |
| [`fileWriteTo`](#filewriteto) | `fileWriteTo(handle, content) -> bool` | Writes through an open file |
| [`fileClose`](#fileclose) | `fileClose(handle) -> bool` | Closes an open file |

Paths, modes and content are `text`. A handle has type `file`.

Calling any built-in with the wrong number of arguments gives `built-in argument count mismatch`. Using a built-in name for your own variable or function gives `built-in names cannot be redefined or shadowed`.

## Console

### ask

`ask(type) -> type`

Reads one line typed by the user and turns it into a value of the named type. The type must be `int`, `real`, `bool` or `text`.

```ruva
launch() {
    show("How many?");
    keep count = ask(int);
    show(count * 2);
}
```

```sh
printf '21\n' | ./a.out
```

```text
How many?
42
```

Spaces around `int`, `real` and `bool` answers are ignored. A `text` answer keeps its spaces. More detail is in [Input and files](../programs/input-and-files.md).

**Errors.** Compile time: `expected a type` for `ask("name")` or `ask()`, and `type is not allowed here` for `ask(file)`. Runtime: `invalid integer text`, `integer text out of range`, `invalid real text`, `real text out of range`, `invalid boolean text` and `unexpected end of input`.

### show

`show(value, ...) -> void`

Prints its values on one line with a space between them, then ends the line. With no values it prints an empty line. Values can be `int`, `real`, `bool` or `text`.

```ruva
launch() {
    show("Total:", 42, 2.5, true);
    show();
    show(0.1, 4.0);
}
```

```text
Total: 42 2.5 true

0.10000000000000001 4
```

A `real` prints with up to 17 significant digits, which is why `0.1` shows its tiny rounding error. Whole `real` values like `4.0` print without the `.0`.

`show` works out every value before printing anything. If one of them fails, nothing from that call is printed.

**Errors.** Compile time: `show accepts only primitive values` for arrays, records, file handles and calls that return nothing.

## Conversions

### toInt

`toInt(value: real) -> int` and `toInt(value: text) -> int`

From a `real`, it drops the decimal part, moving toward zero. From `text`, it reads a whole number, ignoring spaces around it.

```ruva
launch() {
    show(toInt(3.9), toInt(-3.9), toInt(" 42 "));
}
```

```text
3 -3 42
```

**Errors.** Compile time: `conversion does not accept this input type` for an `int` or `bool` argument. Runtime: `invalid integer text`, `integer text out of range`, and `real value cannot convert to int` when the `real` is too big for `int`.

### toReal

`toReal(value: int) -> real` and `toReal(value: text) -> real`

```ruva
launch() {
    keep total = 7;
    show(toReal(total) / 2.0, toReal("2.5"));
}
```

```text
3.5 2.5
```

**Errors.** Compile time: `conversion does not accept this input type` for a `real` or `bool` argument. Runtime: `invalid real text` and `real text out of range`.

### toText

`toText(value: int) -> text`, `toText(value: real) -> text` and `toText(value: bool) -> text`

Gives the same text that `show` would print for that value.

```ruva
launch() {
    keep age = 21;
    show("Age: " + toText(age), toText(2.5) + "!", toText(true));
}
```

```text
Age: 21 2.5! true
```

**Errors.** Compile time: `conversion does not accept this input type` for a `text` argument.

### toBool

`toBool(value: text) -> bool`

Accepts `"true"` or `"false"` in lowercase, with optional spaces around them.

```ruva
launch() {
    show(toBool("true"), toBool(" false "));
}
```

```text
true false
```

**Errors.** Compile time: `conversion does not accept this input type` for anything but `text`. Runtime: `invalid boolean text` for anything else, including `"True"` and `"yes"`.

## Arrays

### push

`push(array, value) -> void`

Adds `value` to the end of the array. The array must be a `vary` variable, and the value must match the element type.

```ruva
launch() {
    vary items: int[] = [];
    push(items, 10);
    push(items, 20);
    show(length(items));
}
```

```text
2
```

**Errors.** Compile time: `binding is read-only` for a `keep` array, `push element type mismatch`, `array built-in requires an array` when the first argument isn't an array, `push and pop require an array binding` when it is an array literal instead of a variable, and `cannot resize or replace an active array walk target` inside a walk over the same array.

### pop

`pop(array) -> element`

Removes the last item and gives it back. The array must be a `vary` variable.

```ruva
launch() {
    vary items = [10, 20, 30];
    show(pop(items), length(items));
}
```

```text
30 2
```

**Errors.** Compile time: the same as `push`, apart from the element type check. Runtime: `cannot pop an empty array`.

### length

`length(array) -> int`

Gives the number of items. It works on `keep` and `vary` arrays.

```ruva
launch() {
    keep names = ["Asha", "Rafi", "Mina"];
    show(length(names));
}
```

```text
3
```

**Errors.** Compile time: `array built-in requires an array`, for example `length("abc")`.

More about all three is in [Arrays](../data/arrays.md).

## Files

Relative paths are found from the folder you run the program in. The file built-ins report a wrong argument type at compile time as `file operation argument type mismatch`. The whole story is in [Input and files](../programs/input-and-files.md).

### fileWrite

`fileWrite(path, content) -> bool`

Creates the file, or empties it if it exists, then writes `content`. Gives back `true` on success and `false` if writing failed, for example because the folder doesn't exist.

```ruva
launch() {
    show(fileWrite("hello.txt", "Hello"));
    show(fileRead("hello.txt"));
}
```

```text
true
Hello
```

### fileAppend

`fileAppend(path, content) -> bool`

Adds `content` to the end of the file, creating the file first if needed. Gives back `true` on success and `false` on failure.

```ruva
launch() {
    fileWrite("list.txt", "a");
    fileAppend("list.txt", "b");
    show(fileRead("list.txt"));
}
```

```text
ab
```

### fileRead

`fileRead(path) -> text`

Reads the whole file and gives it back as text, line breaks included.

```ruva
launch() {
    fileWrite("note.txt", "remember the milk");
    keep content = fileRead("note.txt");
    show(content);
}
```

```text
remember the milk
```

**Errors.** Runtime: `cannot open file` when the file is missing, and `invalid UTF-8 text` when it doesn't hold valid text.

### fileExists

`fileExists(path) -> bool`

Gives `true` when a regular file is at that path. A folder gives `false`.

```ruva
launch() {
    show(fileExists("nothing-here.txt"));
    fileWrite("here.txt", "x");
    show(fileExists("here.txt"));
}
```

```text
false
true
```

### fileOpen

`fileOpen(path, mode) -> file`

Opens a file and gives back a handle. The mode is `"read"`, `"write"` or `"append"`. A `"write"` handle empties the file once, when it opens.

```ruva
launch() {
    keep log = fileOpen("log.txt", "write");
    fileWriteTo(log, "started");
    fileClose(log);
    show(fileRead("log.txt"));
}
```

```text
started
```

A handle can't be copied, passed to a function or shown. To store a new handle in the same variable, use `vary` and assign a new `fileOpen(...)` call.

**Errors.** Compile time: `file initialization and rebinding require a fresh fileOpen call` when copying a handle. Runtime: `invalid file mode` and `cannot open file`.

### fileReadFrom

`fileReadFrom(handle) -> text`

Gives back everything in the file that this handle hasn't read yet. At the end of the file it gives empty text. The handle must be opened with `"read"`.

```ruva
launch() {
    fileWrite("data.txt", "1 2 3");
    keep input = fileOpen("data.txt", "read");
    show(fileReadFrom(input));
    fileClose(input);
}
```

```text
1 2 3
```

**Errors.** Runtime: `file handle is not readable`, `file handle is closed` and `invalid UTF-8 text`.

### fileWriteTo

`fileWriteTo(handle, content) -> bool`

Writes `content` after whatever the handle wrote before. The handle must be opened with `"write"` or `"append"`. Gives back `true` on success and `false` if the write failed.

```ruva
launch() {
    keep out = fileOpen("steps.txt", "write");
    show(fileWriteTo(out, "one "), fileWriteTo(out, "two"));
    fileClose(out);
    show(fileRead("steps.txt"));
}
```

```text
true true
one two
```

**Errors.** Runtime: `file handle is not writable` and `file handle is closed`.

### fileClose

`fileClose(handle) -> bool`

Closes the file. Gives back `true` when it closed cleanly and `false` if the system reported a problem. After this the handle can't be used. Handles still open when the program ends are closed for you.

```ruva
launch() {
    keep f = fileOpen("temp.txt", "write");
    show(fileClose(f));
}
```

```text
true
```

**Errors.** Runtime: `file handle is closed` when you close the same handle twice.

Next: [Syntax](syntax.md)
