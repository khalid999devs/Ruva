# Ruva

Ruva is a programming language project with a native compiler named `ruvac`. Source files use the `.ruv` extension. The compiler is being built in C11 with Flex for tokenization, C output, and GCC or Clang for native compilation.

**Status:** compiler foundations are implemented: source buffers, spans, diagnostics, an arena, a vector and an isolated fixture runner. The lexer and later stages are next; a `ruvac` executable is not available yet.

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

A C11 compiler, Make and Python 3.9 or later are needed for the current build and tests. Flex is needed for the upcoming lexer. Git manages source history.

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

The default build produces `build/debug/lib/libruva_foundation.a` and foundation test programs. `make test` runs C checks, seven program fixtures, and Python tooling tests. `make sanitize` builds and runs the C checks and fixtures with AddressSanitizer and UndefinedBehaviorSanitizer on supported toolchains. `make clean` removes generated build output.

Compiler and flags can be overridden, using a separate output directory when they change:

```sh
make CC=clang BUILD_DIR=build/clang test
```

The test probe exercises foundation APIs; it does not compile Ruva programs yet.

## Repository organization

| Path | Purpose |
| --- | --- |
| `.githooks/` | Commit and push checks |
| `scripts/repository/` | Repository checks |
| `tests/repository/` | Tests for the repository guards |
| `compiler/common/` | Platform checks, allocation, source handling and diagnostics |
| `scripts/testing/` | Isolated program fixture runner |
| `tests/foundation/` | C unit checks and fixture probe |
| `tests/fixtures/foundation/` | Source, diagnostic, I/O and allocation fixtures |
| `tests/testing/` | Fixture runner tests |
| `runtime/`, `examples/` | Added with later working language slices |

Keep no more than six direct project files in a directory. Group new files into subdirectories by responsibility before exceeding that limit.

Prefer clear names and focused functions. Add short comments only when a process or decision needs an explanation.
