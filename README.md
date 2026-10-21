# Ruva

Ruva is a programming language project with a native compiler named `ruvac`. Source files use the `.ruv` extension. The compiler is being built in C11 with Flex for tokenization, C output, and GCC or Clang for native compilation.

**Status:** the lexer and recursive-descent parser are implemented, including syntax trees, precedence, declarations and control flow. Name resolution, type checking and native compilation are next; a `ruvac` executable is not available yet.

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

The default build produces foundation, lexer and parser libraries, C tests and inspection tools under `build/debug/`. `make test` runs three C suites, 40 program fixtures and Python tooling tests. `make sanitize` builds and runs the C checks and fixtures with AddressSanitizer and UndefinedBehaviorSanitizer on supported toolchains. `make clean` removes generated build output.

Compiler and flags can be overridden, using a separate output directory when they change:

```sh
make CC=clang BUILD_DIR=build/clang test
```

The inspection tools show tokens and syntax trees. They do not resolve names, check types or produce native programs yet.

## Inspection tools

```sh
build/debug/tests/lexer/lexer-probe tests/fixtures/lexer/literals/input.ruv
```

The lexer probe prints token kinds, source positions, original spellings and decoded text. Its `--check` option checks lexical validity only.

```sh
build/debug/tests/parser/parser-probe tests/fixtures/parser/expression/input.ruv
build/debug/tests/parser/parser-probe --module --check tests/fixtures/parser/module/input.ruv
```

The parser probe prints an abstract syntax tree. `--module` selects imported-module syntax and `--check` checks syntax without printing the tree. Semantic errors are checked by later compiler stages.

## Repository organization

| Path | Purpose |
| --- | --- |
| `.githooks/` | Commit and push checks |
| `scripts/repository/` | Repository checks |
| `tests/repository/` | Tests for the repository guards |
| `compiler/common/` | Platform checks, allocation, source handling, diagnostics and byte-oriented stream setup |
| `compiler/frontend/lexer/`, `tests/lexer/`, `tests/fixtures/lexer/` | Lexer, inspection tool and positive/negative checks |
| `compiler/frontend/ast/`, `compiler/frontend/parser/`, `compiler/analysis/types/type.h` | Syntax trees, recursive-descent parser and shared type declarations |
| `tests/parser/`, `tests/fixtures/parser/` | Parser structure, syntax, ambiguity and allocation-failure checks |
| `scripts/testing/` | Isolated program fixture runner |
| `tests/foundation/` | C unit checks and fixture probe |
| `tests/fixtures/foundation/` | Source, diagnostic, I/O and allocation fixtures |
| `tests/testing/` | Fixture runner tests |
| `runtime/`, `examples/` | Added with later working language slices |

Keep no more than six direct project files in a directory. Group new files into subdirectories by responsibility before exceeding that limit.

Prefer clear names and focused functions. Add short comments only when a process or decision needs an explanation.
