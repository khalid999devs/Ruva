# Keywords

Ruva reserves 25 words. You can't use any of them as the name of a variable, constant, function, parameter or record type.

## All keywords

| Keyword | What it does | Learn more |
| --- | --- | --- |
| `launch` | Marks the entry point, `launch() { ... }`, where the program starts | [Program structure](../basics/program.md#the-entry-point) |
| `keep` | Declares a read-only variable or a top-level constant | [Variables](../basics/variables.md) |
| `vary` | Declares a variable that can change | [Variables](../basics/variables.md) |
| `share` | Makes a top-level function, record type or constant visible to other files | [Modules](../programs/modules.md) |
| `bring` | Imports shared names from another file | [Modules](../programs/modules.md) |
| `craft` | Defines a function | [Functions](../flow/functions.md) |
| `give` | Ends a function, handing back a value if it has one | [Functions](../flow/functions.md#give) |
| `object` | Declares a record type with named fields | [Records](../data/records.md) |
| `int` | Type of whole numbers | [Values and types](../basics/values.md#int) |
| `real` | Type of floating-point numbers | [Values and types](../basics/values.md#real) |
| `bool` | Type of `true` and `false` | [Values and types](../basics/values.md#bool) |
| `text` | Type of text | [Values and types](../basics/values.md#text) |
| `void` | Return type of a function that gives no value | [Functions](../flow/functions.md#void-functions) |
| `file` | Type of an open file handle | [Input and files](../programs/input-and-files.md) |
| `true` | The `bool` value true | [Values and types](../basics/values.md#bool) |
| `false` | The `bool` value false | [Values and types](../basics/values.md#bool) |
| `check` | Runs a block when a condition is true | [Decisions](../flow/decisions.md#check) |
| `otherwise` | The other branch of a `check`, or the default case of a `choose` | [Decisions](../flow/decisions.md#check-with-otherwise) |
| `choose` | Picks one case by comparing a value against literals | [Decisions](../flow/decisions.md#choose) |
| `when` | One case inside a `choose` | [Decisions](../flow/decisions.md#choose) |
| `walk` | Starts a loop over a range, an array or a record's fields | [Loops](../flow/loops.md) |
| `from` | Gives the start of a `walk` range. Also used in `bring { ... } from "file.ruv";` | [Loops](../flow/loops.md#walk-from-until) |
| `until` | Gives the end of a `walk` range, which is excluded | [Loops](../flow/loops.md#walk-from-until) |
| `in` | Names the array or record a `walk` goes through | [Loops](../flow/loops.md#walk-in-an-array) |
| `during` | Repeats a block while a condition is true | [Loops](../flow/loops.md#during) |

## Using a keyword as a name

Every keyword is rejected where a name is expected:

```ruva
launch() {
    keep walk = 1;
}
```

```text
main.ruv:2:10: error: expected a binding name
```

## Case matters

Keywords are all lowercase. A different spelling is an ordinary name, so `Launch`, `Keep` and `TEXT` are allowed. They are easy to confuse with the real keywords, though, so it is better to avoid them.

## Words that are not keywords

- Words common in other languages, such as `if`, `else`, `while`, `for`, `return` and `break`, have no meaning in Ruva. They are ordinary names, so writing `if x > 1 { ... }` gives a confusing error instead of a branch. Use `check`, `otherwise`, `during`, `walk` and `give`.
- `key` and `value` are not keywords. They are only special after a record field walk variable, as in `field.key`.
- The 17 built-in function names, such as `show`, `ask` and `length`, are not keywords, but you can't use them as names either. See [Built-in functions](builtins.md).

Next: [Built-in functions](builtins.md)
