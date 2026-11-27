<p align="center">
  <img src=".github/assets/banner.png" alt="Ruva" width="100%">
</p>

<p align="center">
  <a href="https://khalid999devs.github.io/Ruva/"><img src="https://img.shields.io/badge/docs-khalid999devs.github.io%2FRuva-4338CA" alt="Documentation"></a>
  <img src="https://img.shields.io/badge/written%20in-C11-2DD4BF" alt="Written in C11">
  <img src="https://img.shields.io/badge/platforms-macOS%20%7C%20Linux-64748B" alt="macOS and Linux">
  <img src="https://img.shields.io/badge/tests-177%20programs%20passing-34D399" alt="177 test programs passing">
  <a href="LICENSE"><img src="https://img.shields.io/badge/license-MIT-8B5CF6" alt="MIT License"></a>
</p>

<p align="center">
  <a href="https://khalid999devs.github.io/Ruva/"><b>Documentation</b></a> ·
  <a href="https://khalid999devs.github.io/Ruva/start/install/">Install</a> ·
  <a href="https://khalid999devs.github.io/Ruva/basics/program/">Language tour</a> ·
  <a href="https://khalid999devs.github.io/Ruva/reference/builtins/">Built-ins</a> ·
  <a href="https://khalid999devs.github.io/Ruva/contributing/">Contributing</a>
</p>

Ruva is a small statically typed programming language with its own compiler, `ruvac`. The compiler reads `.ruv` files, checks them, turns them into C and builds a native executable with GCC or Clang. Install it once and use it from any folder, the same way you use `gcc`.

<p align="center">
  <img src=".github/assets/install.gif" alt="Installing ruvac, writing a program and compiling it" width="760">
</p>

## Features

- Static types with `int`, `real`, `bool` and `text`, plus records and arrays
- Functions with parameters, return values and recursion
- `check` and `otherwise`, `choose` and `when`, `walk` loops and `during` loops
- Records with field access, copies and walks over their fields
- Arrays with `push`, `pop`, `length` and checked indexing
- Modules across several files with `share` and `bring`
- Typed keyboard input, conversions and file operations
- Checked integer arithmetic and errors that point to the exact line and column

## Install

You need a C11 compiler (GCC or Clang), Flex, Make and Git. Python 3.9 or later is only needed for the tests.

```sh
git clone https://github.com/khalid999devs/Ruva.git
cd Ruva
make
sudo ln -s "$PWD/build/debug/bin/ruvac" /usr/local/bin/ruvac
```

`ruvac` now works from any folder. Keep the cloned folder, since the compiler uses the runtime sources inside it. To install without `sudo`, see the [install guide](https://khalid999devs.github.io/Ruva/start/install/).

## Write and compile a program

```sh
mkdir ~/hello && cd ~/hello
```

Save this as `main.ruv`:

```
craft square(n: int) -> int {
    give n * n;
}

launch() {
    show("7 squared is", square(7));
}
```

```sh
ruvac main.ruv -o square
./square
```

```
7 squared is 49
```

## Examples

The repository comes with working programs in [`examples`](examples).

<p align="center">
  <img src=".github/assets/demo.gif" alt="Compiling and running the Ruva examples" width="760">
</p>

| Example | What it shows | Output |
| --- | --- | --- |
| [`hello`](examples/hello/main.ruv) | Entry point and `show` | `Hello from Ruva` |
| [`functions`](examples/functions/main.ruv) | Recursion and a counted loop | `120 rururu` |
| [`collections`](examples/collections/main.ruv) | Arrays, records and walks | `12 3` and the record fields |
| [`io`](examples/io/main.ruv) | Keyboard input and file writing | Writes and reads `message.txt` |
| [`modules`](examples/modules/main.ruv) | Four files sharing a record | `Asha: 42 (pass)` |

## Using the compiler

```sh
ruvac main.ruv                          # check, emit C and build a.out
ruvac main.ruv -o app                   # pick the executable name
ruvac --check main.ruv                  # check the program and its imports only
ruvac --emit-c main.ruv -o main.c       # write the generated C
ruvac --sanitize main.ruv -o app        # build with AddressSanitizer and UBSan
```

A successful compile prints nothing, and errors show the file, line and column. For a program split into modules, compile only the entry file and `ruvac` follows its `bring` imports.

## How it works

```
.ruv source  →  tokens  →  AST  →  modules  →  type checks  →  C  →  native executable
```

| Stage | Location |
| --- | --- |
| Lexer built with Flex | [`compiler/frontend/lexer`](compiler/frontend/lexer) |
| Recursive descent parser and AST | [`compiler/frontend/parser`](compiler/frontend/parser), [`compiler/frontend/ast`](compiler/frontend/ast) |
| Module graph and type checks | [`compiler/analysis`](compiler/analysis) |
| C code generation | [`compiler/backend/c`](compiler/backend/c) |
| Command line and native build | [`compiler/driver`](compiler/driver) |
| Runtime for text, arrays, input and files | [`runtime`](runtime) |

## Testing

```sh
make test       # C suites, 177 program fixtures and the Python tooling tests
make sanitize   # the C and program tests with AddressSanitizer and UBSan
make clean
```

## Contributing

Ruva is a learning project and contributions are welcome. The whole compiler, from the Flex lexer to the C generator and the runtime, is small enough to read end to end, so fixing an error message, adding a test or improving the docs is a good way to learn how compilers work. Start with the [contributor guide](https://khalid999devs.github.io/Ruva/contributing/), and feel free to open an issue with questions.

## License

Ruva is released under the [MIT License](LICENSE). Built by [khalid999devs](https://github.com/khalid999devs).
