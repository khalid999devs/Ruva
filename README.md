# Ruva

Ruva is a programming language project with a native compiler named `ruvac`. Source files use the `.ruv` extension. The compiler is implemented in C11 with Flex for tokenization, C output, and GCC or Clang for native compilation.

**Status:** `ruvac` checks Ruva programs, emits C, and builds native executables with functions, recursion, control flow, primitive arrays, records, and collection walks. Checked arithmetic, managed text, record copies and arguments/results, ordered expressions, and `show` work. All 17 built-ins are implemented, including typed input, conversions, and file operations. The implementation has passed release checks on macOS and Linux with strict C warnings and sanitizers.

## Intended flow

```text
.ruv -> tokens -> AST -> module resolution -> type checks -> C -> native executable
```

A minimal Ruva program uses the existing entry point and console built-in:

```text
launch() {
    show("Hello from Ruva");
}
```

## Development setup

The build needs a C11 compiler, Flex, Make, and Python 3.9 or later. It is verified on arm64 macOS with Apple Clang 21 and arm64 Debian 12 with GCC 12.2 and Clang 14. A native Windows x64 implementation and test command are prepared; execution on Windows is still pending. Git manages source history. This release uses a source checkout; native builds use its runtime sources and shared host support. Clang sanitizer builds need the matching compiler-rt libraries.

After cloning, enable the tracked Git hooks:

```sh
git config core.hooksPath .githooks
python3 scripts/repository/check-layout.py
python3 -m unittest discover -s tests/repository -v
```

The hooks run repository checks before commits and pushes. Git does not enable repository hooks automatically in a fresh clone, so the configuration command is required there.

## Build and test

```sh
make
make test
make sanitize
make clean
```

On macOS/Linux, the default build produces `build/debug/bin/ruvac`, compiler/runtime libraries under `build/debug/lib/`, and test programs. Flex generates its C scanner under `build/`. `make test` runs seven C suites, 177 program fixtures, and 76 Python tooling/integration tests. `make sanitize` instruments the compiler, runtime, generated native programs, and integration checks with AddressSanitizer and UndefinedBehaviorSanitizer on supported toolchains. `make clean` removes generated build output.

For an extracted source ZIP without Git metadata, use `make test-programs` to run those same C, fixture and Python tests. Git must still be installed for the tooling tests, which create isolated temporary repositories. `make test` additionally checks the current repository index; use it in a Git checkout. `make sanitize` works from either distribution.

Compiler and flags can be overridden, using a separate output directory when they change:

```sh
make CC=clang BUILD_DIR=build/clang test
make CC=gcc BUILD_DIR=build/gcc CFLAGS="-O2 -g" test
make CC=gcc SANITIZE_BUILD_DIR=build/gcc-sanitize sanitize
```

## Windows preparation

The first Windows target is x64 with [MSYS2 UCRT64 GCC](https://www.msys2.org/docs/environments/). Build and link checks from another host pass; real Windows acceptance tests have not been run yet. MSVC, Windows ARM64, and Windows GCC sanitizers are outside this initial target.

Install and update [MSYS2](https://www.msys2.org/), then open its **UCRT64** terminal. Install the tools:

```sh
pacman -S --needed make flex mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-python
```

From the project root, build and run the Windows checks:

```sh
make -j4 CC=gcc PYTHON=python BUILD_DIR=build/windows windows-test
```

This builds `build/windows/bin/ruvac.exe`. The Windows runner checks four C suites, 152 byte-exact fixtures, 10 shared module integration tests, and 12 Windows-specific integration tests. It saves the commands, host/toolchain details, results, and failures under `build/windows/verification.log` and `verification.json`. POSIX descriptor/FIFO stress suites remain separate. On this target, `make test` selects the Windows runner too.

After building, PowerShell can invoke the compiler directly. Adjust the GCC path if MSYS2 was installed elsewhere:

```powershell
$env:Path = "C:\msys64\ucrt64\bin;$env:Path"
$env:CC = 'C:\msys64\ucrt64\bin\gcc.exe'
.\build\windows\bin\ruvac.exe examples\hello\main.ruv -o build\hello.exe
.\build\hello.exe
```

The approved language is identical across targets. Windows uses `a.exe` as the default native output. Use `.exe` for explicit executable output names. `CC` must name one compiler executable; batch files and shell command strings are unsupported. Keep the source checkout together so native compilation can find the runtime and shared host files.

## Compile a program

```sh
build/debug/bin/ruvac examples/hello/main.ruv -o build/hello
./build/hello
```

This prints `Hello from Ruva`. A successful compilation is silent.

A function and loop example is available too:

```sh
build/debug/bin/ruvac examples/functions/main.ruv -o build/functions
./build/functions
```

This prints `120 rururu`, using recursive factorial and a text-building range loop.

An array and record example is available too:

```sh
build/debug/bin/ruvac examples/collections/main.ruv -o build/collections
./build/collections
```

It totals a real array, passes and returns a record, then prints its fields:

```text
12 3
label sample
value 12
valid true
```

The input and file example writes `message.txt` in its current directory:

```sh
build/debug/bin/ruvac examples/io/main.ruv -o build/io
cd build
printf 'Ruva\n' | ./io
cd ..
```

It reads a message, performs two sequential writes through one handle, and reads the result back. The file contains `Ruva`, CRLF, and `8`.

A four-file module example shares a record between two services:

```sh
build/debug/bin/ruvac examples/modules/main.ruv -o build/modules
./build/modules
```

This prints `Asha: 42 (pass)`. Compile the entry file once; the compiler follows its imports and includes the dependencies in one executable. Each import path is relative to the file containing it. Publish functions, records and literal constants with `share`, then select them with `bring { name } from "relative/file.ruv";`. Other declarations stay private to their module. Imports use the original names; aliases, wildcards, dependency cycles and automatic re-exports are unsupported.

```sh
build/debug/bin/ruvac --check examples/hello/main.ruv
build/debug/bin/ruvac --emit-c examples/hello/main.ruv -o build/hello.c
build/debug/bin/ruvac --sanitize examples/hello/main.ruv -o build/hello-sanitized
```

`--check` validates the full language and its imports without invoking a C compiler. Imported functions/constants and primitive/text/record arguments/results work. Arrays support push/pop, checked indexing, fresh replacement, and walks over current elements; record walks preserve entry-time field snapshots. Function parameters and walk bindings remain read-only. Typed `ask` reads a complete line; numeric conversions validate decimal spelling and range. File operations preserve text bytes, check access modes and closed handles, and close remaining resources at exit. Default output names are `a.out` on macOS/Linux, `a.exe` on Windows, and `out.c` for C emission.

Native compilation needs runtime sources from this checkout. The driver locates them relative to its executable; `RUVA_RUNTIME_ROOT` can point to the checkout's `runtime` directory. `CC` selects one C compiler executable, including a path with spaces. It is passed directly as an executable path; shell command strings and flag lists are not accepted. Generated C also needs the runtime headers and implementations when compiled separately.

Compiler resource limits produce positioned errors, including a 16 MiB limit on generated C. Failed checks or native builds preserve existing output files. Successful output replaces the destination after generation/linking completes. Source files and nonregular output destinations are protected. `--sanitize` instruments native output; it does not change Ruva syntax.

## Inspection tools

To inspect the lexer output:

```sh
build/debug/tests/lexer/lexer-probe tests/fixtures/lexer/literals/input.ruv
```

This test tool prints token kinds, positions, original spellings and decoded text. Its `--check` option checks only lexical validity. It does not parse, type-check, resolve imports or compile a program.

To inspect the parsed syntax tree:

```sh
build/debug/tests/parser/parser-probe tests/fixtures/parser/expression/input.ruv
build/debug/tests/parser/parser-probe --module --check tests/fixtures/parser/module/input.ruv
```

The parser tool accepts an entry file by default; `--module` selects an imported module. Its `--check` option checks syntax only. Unknown names, type mismatches and other semantic errors are checked by the semantic tool below. An AST `choice` preserves possible readings of empty header braces until name resolution.

To check a complete program and its imports:

```sh
build/debug/tests/semantic/semantic-probe tests/fixtures/semantic/imports/input.ruv
```

A successful check is silent. `--summary` reports module and symbol counts; `--ast` prints the entry tree after name resolution. `--relative-diagnostics` displays paths under the working directory relatively, which keeps isolated fixture results stable. This tool checks types, built-in signatures, return paths, record constructors, read-only bindings, and array/file ownership. It does not execute Ruva code or perform file operations from the program.

## Repository organization

| Path | Purpose |
| --- | --- |
| `.githooks/` | Commit and push checks |
| `scripts/repository/` | Repository checks |
| `tests/repository/` | Tests for the repository guards |
| `compiler/common/` | Platform checks, allocation, source handling, diagnostics and shared host support |
| `compiler/frontend/lexer/` | Tokens, Flex rules and scanner integration |
| `compiler/frontend/ast/`, `compiler/frontend/parser/` | Syntax tree, inspection output and recursive-descent parser |
| `compiler/analysis/` | Canonical module graph, declaration identities, types and semantic checks |
| `scripts/testing/` | Isolated program fixture runner |
| `scripts/windows/`, `tests/windows/`, `tests/host/` | Windows verification/source packaging, Windows integration checks and shared host tests |
| `tests/foundation/` | C unit checks and fixture probe |
| `tests/fixtures/foundation/` | Source, diagnostic, I/O and allocation fixtures |
| `tests/testing/` | Fixture runner tests |
| `tests/lexer/`, `tests/fixtures/lexer/` | Lexer unit checks, inspection tool and fixtures |
| `tests/parser/`, `tests/fixtures/parser/` | Parser structure/failure checks, inspection tool and fixtures |
| `tests/semantic/`, `tests/fixtures/semantic/` | Semantic, import, ownership and failure checks plus inspection tool |
| `compiler/backend/c/`, `compiler/driver/` | Ordered expressions, functions, collections, CLI and native toolchain invocation |
| `runtime/core/`, `runtime/text/`, `runtime/collections/` | Checked numbers, allocation registry, managed text, primitive arrays, formatting and output |
| `runtime/conversion/`, `runtime/io/`, `runtime/files/` | Decimal parsing, full-line input, byte-preserving files and stable handles |
| `tests/runtime/`, `tests/backend/`, `tests/integration/` | Runtime, emission and driver checks |
| `tests/fixtures/runtime/`, `tests/fixtures/native/` | Fatal-cleanup and native execution fixtures |
| `examples/hello/`, `examples/functions/`, `examples/collections/`, `examples/io/`, `examples/modules/` | Working native examples for core language, collections, input, files and shared modules |

Keep no more than six direct project files in a directory. Group new files into subdirectories by responsibility before exceeding that limit.

Prefer clear names and focused functions. Add short comments only when a process or decision needs an explanation.
