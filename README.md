# Ruva

Ruva is a programming language project with a native compiler named `ruvac`. Source files use the `.ruv` extension. The compiler is being built in C11 with Flex for tokenization, C output, and GCC or Clang for native compilation.

**Status:** the lexer, parser, module loader and semantic checker are implemented. Named imports, scopes, exact types, built-in signatures, return paths and ownership restrictions are checked. C generation and native execution are next; a `ruvac` executable is not available yet.

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

A C11 compiler, Make and Python 3.9 or later are needed for the current build and tests. Flex is needed to generate the lexer. Git manages source history.

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

The default build produces foundation, lexer, parser and analysis libraries, C tests and inspection tools under `build/debug/`. `make test` runs four C suites, 60 program fixtures and Python tooling tests. `make sanitize` builds and runs the C checks and fixtures with AddressSanitizer and UndefinedBehaviorSanitizer on supported toolchains. `make clean` removes generated build output.

Compiler and flags can be overridden, using a separate output directory when they change:

```sh
make CC=clang BUILD_DIR=build/clang test
```

The inspection tools show tokens and syntax trees and validate complete source programs. The semantic probe resolves imports and checks types; native output is not available yet.

## Inspection tools

```sh
build/debug/tests/lexer/lexer-probe tests/fixtures/lexer/literals/input.ruv
```

The lexer probe prints token kinds, source positions, original spellings and decoded text. Its `--check` option checks lexical validity only.

```sh
build/debug/tests/parser/parser-probe tests/fixtures/parser/expression/input.ruv
build/debug/tests/parser/parser-probe --module --check tests/fixtures/parser/module/input.ruv
```

The parser probe prints an abstract syntax tree. `--module` selects imported-module syntax and `--check` checks syntax without printing the tree. Use the semantic probe below to check names and types across imports.

To check a complete program and its imports:

```sh
build/debug/tests/semantic/semantic-probe tests/fixtures/semantic/imports/input.ruv
```

Success is silent. `--summary` reports module and symbol counts; `--ast` prints the checked entry tree. The checker validates shared imports, visibility, exact types, function arguments and returns, read-only bindings, records and array/file ownership. It does not execute programs or perform their input/file operations.

## Repository organization

| Path | Purpose |
| --- | --- |
| `.githooks/` | Commit and push checks |
| `scripts/repository/` | Repository checks |
| `tests/repository/` | Tests for the repository guards |
| `compiler/common/` | Platform checks, allocation, source handling, diagnostics and byte-oriented stream setup |
| `compiler/frontend/lexer/`, `tests/lexer/`, `tests/fixtures/lexer/` | Lexer, inspection tool and positive/negative checks |
| `compiler/frontend/ast/`, `compiler/frontend/parser/` | Syntax trees and recursive-descent parser |
| `compiler/analysis/` | Canonical module graph, symbols, types, scopes and semantic checks |
| `tests/semantic/`, `tests/fixtures/semantic/` | Semantic, import, ownership and allocation-failure checks plus inspection tool |
| `tests/parser/`, `tests/fixtures/parser/` | Parser structure, syntax, ambiguity and allocation-failure checks |
| `scripts/testing/` | Isolated program fixture runner |
| `tests/foundation/` | C unit checks and fixture probe |
| `tests/fixtures/foundation/` | Source, diagnostic, I/O and allocation fixtures |
| `tests/testing/` | Fixture runner tests |
| `runtime/`, `examples/` | Added with later working language slices |

Keep no more than six direct project files in a directory. Group new files into subdirectories by responsibility before exceeding that limit.

Prefer clear names and focused functions. Add short comments only when a process or decision needs an explanation.
