# Modules

A module is a `.ruv` file whose functions, objects and constants other files can use. Modules let you split a bigger program into small files that each do one job.

Two keywords do the work. `share` marks a name as usable from other files. `bring` pulls shared names into the file that needs them.

## A first example

Make a folder with two files:

```text
hello/
├── greet.ruv
└── main.ruv
```

`greet.ruv` shares a constant and a function. Its helper `shout` stays private.

```ruva
share keep greeting = "Hello";

craft shout(word: text) -> text {
    give word + "!";
}

share craft greet(name: text) -> text {
    give greeting + ", " + shout(name);
}
```

`main.ruv` brings in the two shared names and uses them:

```ruva
bring { greet, greeting } from "greet.ruv";

launch() {
    show(greet("Asha"));
    show(greeting);
}
```

Compile the entry file only. `ruvac` finds `greet.ruv` by itself.

```sh
cd hello
ruvac main.ruv
./a.out
```

```text
Hello, Asha!
Hello
```

## What you can share

Put `share` in front of a top-level declaration:

| Declaration | Example |
| --- | --- |
| Function | `share craft greet(name: text) -> text { ... }` |
| Object | `share object Item { ... }` |
| Constant | `share keep greeting = "Hello";` |

Anything without `share` is private to its file. Two modules can each have a private helper with the same name without any clash.

A few things can't be shared:

- A shared constant must be a plain literal, like `40`, `-5`, `2.5`, `"Hello"` or `true`. `share keep total = 1 + 2;` is an error.
- There are no shared variables. `vary` is not allowed at the top level of any file.
- `launch()` can't be shared, and only the entry file may have one.

## Bringing names in

```ruva
bring { greet, greeting } from "greet.ruv";
```

The rules:

- List each name you want inside the braces, separated by commas.
- Every listed name must be shared by that file.
- Only the names you list come in. Nothing else from the file is visible.
- All `bring` lines go at the very top, before any other declaration.
- A brought name can't have the same name as something already declared in your file, or another brought name.

You only need to bring an object's name when you write it yourself, in a type like `item: Item` or a literal like `Item { ... }`. A record returned by a function you brought works without it.

## Paths are relative to the file

The path in `bring` is a text literal. It is read relative to the file that contains the `bring`, not the folder you run `ruvac` from. Write the whole file name, `.ruv` included. Use `..` to go up one folder. Absolute paths like `"/home/me/lib.ruv"` are not allowed.

This program, the same as `examples/modules` in the repository, uses subfolders:

```text
shop/
├── main.ruv
├── model/
│   └── item.ruv
└── services/
    ├── create.ruv
    └── render.ruv
```

`model/item.ruv` holds the shared data:

```ruva
share object Item {
    name: text;
    score: int;
}

share keep pass_score = 40;
```

`services/create.ruv` goes up one folder to reach it:

```ruva
bring { Item } from "../model/item.ruv";

craft helper(score: int) -> int {
    give score + 2;
}

share craft make(name: text, score: int) -> Item {
    give Item { name: name, score: helper(score) };
}
```

`services/render.ruv` brings the same file and has its own private `helper`:

```ruva
bring { Item, pass_score } from "../model/item.ruv";

craft helper(score: int) -> text {
    check score >= pass_score {
        give "pass";
    } otherwise {
        give "retry";
    }
}

share craft describe(item: Item) -> text {
    give item.name + ": " + toText(item.score) + " (" + helper(item.score) + ")";
}
```

`main.ruv` brings one name from each file:

```ruva
bring { Item } from "model/item.ruv";
bring { make } from "services/create.ruv";
bring { describe } from "services/render.ruv";

launch() {
    keep item: Item = make("Asha", 40);
    show(describe(item));
}
```

```sh
ruvac main.ruv
./a.out
```

```text
Asha: 42 (pass)
```

Three files bring `item.ruv` and that is fine. Because paths follow the files, `ruvac shop/main.ruv` from the parent folder works too.

## Compile only the entry file

The entry file is the one with `launch()`. Give `ruvac` that file and it follows every `bring` for you. To check the whole program without building it, use `--check`:

```sh
ruvac --check main.ruv
```

If you pass a module file by mistake, `ruvac` treats it as the entry file and complains:

```text
render.ruv:14:1: error: entry file requires launch()
```

## Not supported

| Idea | Example | What happens |
| --- | --- | --- |
| Renaming on import | `bring { greet as hello } from "greet.ruv";` | `expected '}' after imported names` |
| Bringing everything | `bring * from "greet.ruv";` | `expected '{' after bring` |
| Files that bring each other | `a.ruv` brings from `b.ruv` and `b.ruv` brings from `a.ruv` | `circular import` |
| Passing a name along | `main.ruv` brings `limit` from a file that only brought it | `imported name is not declared by this module` |
| Absolute paths | `bring { greet } from "/tmp/greet.ruv";` | `import path must be nonempty and relative` |

To use a name in several places, bring it from the file that declares it.

## Common mistakes

Bringing a name the module didn't share:

```ruva
bring { shout } from "greet.ruv";

launch() {
    show(shout("hi"));
}
```

```text
main.ruv:1:9: error: imported declaration is private
```

| Message | Usual cause | Fix |
| --- | --- | --- |
| `imported declaration is private` | The name exists but has no `share` | Add `share` in the module |
| `imported name is not declared by this module` | A typo, or the name lives in another file | Check the spelling and the path |
| `cannot resolve source path` | The file doesn't exist at that path | Fix the path, relative to the importing file |
| `imports must precede declarations` | A `bring` below a function or constant | Move every `bring` to the top |
| `duplicate name in this scope` | Two brought names, or a brought and a local name, are the same | Rename one of them |
| `imported modules cannot define launch` | A module file has `launch()` | Keep `launch()` in the entry file only |
| `circular import` | Two files bring from each other | Move the shared part into a third file |

An error inside a module names that module's file, so you know where to look.

Next: [Input and files](input-and-files.md)
