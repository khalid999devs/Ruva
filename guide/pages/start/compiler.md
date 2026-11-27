# Using ruvac

`ruvac` takes one Ruva source file and turns it into a native executable.

```text
usage: ruvac [--check | --emit-c] [--sanitize] source [-o output]
```

## Common commands

| Command | What it does |
| --- | --- |
| `ruvac main.ruv` | Checks the program and builds `a.out` |
| `ruvac main.ruv -o app` | Builds an executable called `app` |
| `ruvac --check main.ruv` | Checks the program and its imports without building anything |
| `ruvac --emit-c main.ruv` | Writes the generated C to `out.c` |
| `ruvac --emit-c main.ruv -o main.c` | Writes the generated C to `main.c` |
| `ruvac --sanitize main.ruv -o app` | Builds with AddressSanitizer and UndefinedBehaviorSanitizer |

A successful run prints nothing. On Windows the default executable name is `a.exe`.

## What happens when you compile

1. `ruvac` reads the source file and every file it brings in with `bring`.
2. It checks names, types and the other language rules.
3. It writes C code for the whole program.
4. It calls a C compiler to build the executable.

If any step fails, `ruvac` prints an error and keeps any existing output file as it was.

## Programs with several files

Compile only the entry file, the one with `launch()`. `ruvac` follows its `bring` lines and builds one executable:

```sh
cd Ruva/examples/modules
ruvac main.ruv && ./a.out
```

```text
Asha: 42 (pass)
```

See [Modules](../programs/modules.md) for how to split a program.

## Choosing the C compiler

`ruvac` uses your system C compiler by default. Set `CC` to pick another one:

```sh
CC=clang ruvac main.ruv -o app
CC=gcc ruvac main.ruv -o app
```

`CC` must name a single compiler executable, not a command with flags. If it cannot be started you get:

```text
ruvac: error: cannot start C compiler: No such file or directory
```

## Errors and exit codes

Problems in your program are reported as `file:line:column: error: message`, and `ruvac` exits with status 1. Problems with the command itself start with `ruvac: error:`. For example, `ruvac` refuses to overwrite your source:

```text
ruvac: error: output would overwrite a source file
```

[Errors](../programs/errors.md) lists the messages you are most likely to see and what they mean.

## Generated C

`--emit-c` is useful when you want to see how a Ruva feature turns into C. The generated file needs the Ruva runtime headers and sources, so compiling it by hand is mostly useful while working on the compiler itself.

Next: [Program structure](../basics/program.md)
