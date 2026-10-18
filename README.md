# Ruva

Ruva is a programming language project with a native compiler named `ruvac`. Source files use the `.ruv` extension. The planned compiler is written in C11, uses Flex for tokenization, emits C, and invokes GCC or Clang to produce an executable.

**Status:** repository setup is complete; the compiler and runtime are not implemented yet. Build commands will be added with the first working compiler slice.

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

The planned toolchain needs a C11 compiler, Flex and Make. Python 3.9 or later runs the repository checks; Git manages source history.

After cloning, enable the tracked Git hooks:

```sh
git config core.hooksPath .githooks
python3 scripts/repository/check-layout.py
python3 -m unittest discover -s tests/repository -v
```

The hooks run repository checks before commits and pushes. Git does not enable repository hooks automatically in a fresh clone, so the configuration command is required there.

## Repository organization

| Path | Purpose |
| --- | --- |
| `.githooks/` | Commit and push checks |
| `scripts/repository/` | Repository checks |
| `tests/repository/` | Tests for the repository guards |
| `compiler/`, `runtime/` | Added as compiler work begins |
| `examples/`, compiler test directories | Added with working language slices |

Keep no more than six direct project files in a directory. Group new files into subdirectories by responsibility before exceeding that limit.

Prefer clear names and focused functions. Add short comments only when a process or decision needs an explanation.
