# Input and files

A program becomes much more useful once it can read what the user types and keep data in files. This page covers `ask` for keyboard input and the eight file built-ins.

## Reading input with ask

`ask(type)` waits for the user to type one line, then turns it into a value of the type you name.

```ruva
launch() {
    show("What is your name?");
    keep name = ask(text);
    show("How old are you?");
    keep age = ask(int);
    show("Your height in meters?");
    keep height = ask(real);
    show("Do you like tea? (true or false)");
    keep likesTea = ask(bool);

    show(name, "is", age, "years old and", height, "m tall.");
    show("Likes tea:", likesTea);
}
```

When you run it, you type each answer after its question. You can also feed the answers in from the shell, one per line:

```sh
printf 'Asha Rahman\n21\n1.75\ntrue\n' | ./a.out
```

```text
What is your name?
How old are you?
Your height in meters?
Do you like tea? (true or false)
Asha Rahman is 21 years old and 1.75 m tall.
Likes tea: true
```

Piped answers don't appear in the output, only what the program shows.

### What each type accepts

Every `ask` reads exactly one whole line. Two numbers on one line are not two answers.

| Call | Accepts | Examples that work |
| --- | --- | --- |
| `ask(int)` | A whole number in the `int` range, with an optional sign | `42`, `+8`, `-17`, and `42` with spaces around it |
| `ask(real)` | A decimal number, with an optional sign and exponent | `2.5`, `3`, `.5`, `1e3`, `-0.25` |
| `ask(bool)` | `true` or `false` in lowercase | `true`, `false`, and either with spaces around it |
| `ask(text)` | Any line | `hello world` with its spaces kept, an empty line |

For `int`, `real` and `bool`, spaces around the answer are ignored. For `text`, the line is kept exactly as typed, spaces included. Only the line ending is removed.

The type inside `ask(...)` must be `int`, `real`, `bool` or `text`. You can't ask for a file, an array or a record.

### When the input is wrong

If the line can't be turned into the type you asked for, the program stops with an error and exit code 1:

```ruva
launch() {
    show("Pick a number:");
    keep n = ask(int);
    show(n * 2);
}
```

```sh
printf '12abc\n' | ./a.out
```

```text
Pick a number:
main.ruv:3:14: error: invalid integer text
```

| Message | Cause |
| --- | --- |
| `invalid integer text` | Not a whole number, for example `12abc`, `3.5`, `0x10` or an empty line |
| `integer text out of range` | A whole number too big for `int`, like `2147483648` |
| `invalid real text` | Not a decimal number, for example `abc`, `1,5` or `inf` |
| `real text out of range` | A number too large to store, like `1e999` |
| `invalid boolean text` | Anything other than `true` or `false`, including `True` and `yes` |
| `unexpected end of input` | The input ran out before the program got its line |

## Files

Ruva reads and writes text files. File paths are text, and a relative path like `"notes.txt"` is found in the folder you run the program from, not the folder that holds the source.

There are two ways to work with files. The whole-file built-ins do one job in one call. File handles let you open a file once and read or write it in steps.

### Whole-file built-ins

```ruva
launch() {
    keep saved = fileWrite("notes.txt", "first line\n");
    keep added = fileAppend("notes.txt", "second line\n");
    show(saved, added);

    show(fileExists("notes.txt"), fileExists("missing.txt"));
    show(fileRead("notes.txt"));
}
```

```text
true true
true false
first line
second line

```

| Built-in | What it does | Gives back |
| --- | --- | --- |
| `fileWrite(path, content)` | Creates the file, or empties it first if it exists, then writes | `bool`, `true` when it worked |
| `fileAppend(path, content)` | Creates the file if needed, then adds to the end | `bool`, `true` when it worked |
| `fileRead(path)` | Reads the whole file | `text` |
| `fileExists(path)` | Checks for a regular file at that path | `bool`, `false` for folders |

`fileWrite` replaces what was there. `fileAppend` keeps it:

```ruva
launch() {
    fileWrite("log.txt", "one\n");
    fileWrite("log.txt", "two\n");
    fileAppend("log.txt", "three\n");
    show(fileRead("log.txt"));
}
```

```text
two
three

```

As you can see, you may ignore the `bool` result. When a write fails, for example because the folder doesn't exist, you get `false` and the program keeps going. Reading a file that isn't there is different. `fileRead` stops the program with `cannot open file`, so check first when you're not sure:

```ruva
launch() {
    check fileExists("settings.txt") {
        show(fileRead("settings.txt"));
    } otherwise {
        show("No settings yet.");
    }
}
```

```text
No settings yet.
```

### File handles

`fileOpen(path, mode)` opens a file and gives back a handle of type `file`. You pass the handle to the other three built-ins, then close it.

```ruva
launch() {
    keep out = fileOpen("todo.txt", "write");
    fileWriteTo(out, "buy milk\n");
    fileWriteTo(out, "call Rafi\n");
    show(fileClose(out));

    keep more = fileOpen("todo.txt", "append");
    fileWriteTo(more, "water plants\n");
    fileClose(more);

    keep input = fileOpen("todo.txt", "read");
    show(fileReadFrom(input));
    show("[" + fileReadFrom(input) + "]");
    fileClose(input);
}
```

```text
true
buy milk
call Rafi
water plants

[]
```

The mode is one of three words:

| Mode | What happens at open | Allowed calls |
| --- | --- | --- |
| `"read"` | The file must exist | `fileReadFrom` |
| `"write"` | The file is created, or emptied once if it exists | `fileWriteTo` |
| `"append"` | The file is created if needed, and writes go to the end | `fileWriteTo` |

How the handle built-ins behave:

- `fileWriteTo(handle, content)` writes after whatever this handle wrote before. It gives back `true` when the write worked and `false` when it failed.
- `fileReadFrom(handle)` gives back everything not read yet. A second call gets empty text, as the `[]` line above shows.
- `fileClose(handle)` closes the file and gives back `true` when that worked. After closing, the handle can't be used again.
- A handle still open when the program ends is closed for you. Closing it yourself as soon as you're done is still a good habit.

Handles follow strict rules, a lot like [arrays](../data/arrays.md):

- A handle can't be copied. `keep copy = out;` is an error.
- Functions can't take or return handles, and `show` can't print one.
- A `keep` handle can still be closed. To point a variable at a new file, declare it with `vary` and assign a new `fileOpen(...)` call.

```ruva
launch() {
    vary f = fileOpen("a.txt", "write");
    fileWriteTo(f, "A");
    fileClose(f);
    f = fileOpen("b.txt", "write");
    fileWriteTo(f, "B");
    fileClose(f);
    show(fileRead("a.txt"), fileRead("b.txt"));
}
```

```text
A B
```

## File errors

These stop the program with exit code 1:

| Message | Cause |
| --- | --- |
| `cannot open file` | `fileRead` or `fileOpen(..., "read")` on a missing file, or a path that can't be opened |
| `invalid file mode` | A mode other than `"read"`, `"write"` or `"append"`, such as `"w"` |
| `file handle is not readable` | `fileReadFrom` on a write or append handle |
| `file handle is not writable` | `fileWriteTo` on a read handle |
| `file handle is closed` | Using or closing a handle after `fileClose` |
| `invalid UTF-8 text` | The file holds bytes that aren't valid text |

Using the wrong mode looks like this:

```ruva
launch() {
    keep f = fileOpen("todo.txt", "w");
}
```

```text
main.ruv:2:14: error: invalid file mode
```

At compile time, passing the wrong kind of value, like `fileWrite(1, "x")`, gives `file operation argument type mismatch`. Copying a handle gives `file initialization and rebinding require a fresh fileOpen call`.

Next: [Errors](errors.md)
