# Workflow and code style

This page covers how the code is written, the rules the repository enforces, how to write commit messages and what a pull request needs before it is merged. It also explains how to propose changes to the language itself.

## Code style

The C code reads the same across the whole repository. Before you change a folder, read a couple of its files and match them. These are the main rules:

- Write C11 without compiler extensions. Every file must build with `-std=c11 -Wall -Wextra -Wpedantic -Werror` on both GCC and Clang.
- Indent with four spaces, never tabs. The `Makefile` is the only exception. `.editorconfig` sets this together with UTF-8, LF line endings, a final newline and no trailing whitespace. Most editors read it on their own or through an EditorConfig plugin.
- Use snake_case names. Compiler and runtime functions that other files call start with `rv_` and usually name their stage, as in `rv_lex_source`, `rv_parse_module`, `rv_check_program` and `rv_c_emit`. Types are `Rv` plus PascalCase, such as `RvToken` or `RvCheckedProgram`. Enum values are uppercase with `RV_`, such as `RV_TOKEN_EOF` or `RV_CHECK_TYPE`.
- Make helpers that only one file uses `static`, with no prefix. `parse_file` in `compiler/analysis/modules/loader.c` and `find_runtime` in `compiler/driver/toolchain/toolchain.c` are examples.
- Give every header an include guard named after it, such as `RUVA_LEXER_H`. Declarations shared only inside one stage go in that folder's `internal.h`, not in its public header.
- Include the file's own header first, then other project headers by their path from the repository root (`"compiler/common/memory/arena.h"`), then system headers. Runtime files are the exception. They include each other with relative paths, because generated programs are compiled with only the `runtime` folder on the include path.
- Keep functions focused on one job. When one grows long or deeply nested, pull out a helper with a clear name.
- Prefer plain code over clever code. Put one statement on each line, leave a blank line between the steps of a function, and reach for an `if` before a nested `?:` or a macro trick.
- Follow the error pattern of the stage you are in. Functions return `bool`, fill in an error struct with a kind, a span and a message, and leave their outputs safe to destroy when they fail. Allocate through the `RvAllocator` you were given, and do size arithmetic with `rv_size_add` and `rv_size_multiply`.
- Write a comment only when a decision needs explaining and the code cannot say it by itself. `/* Flex's fatal path must return through our cleanup boundary. */` in `scanner.l` is a good example. Leave out comments that repeat the code, and do not add decorative banners or separator lines.
- Python scripts and tests use only the standard library, four-space indentation, `argparse` for command lines and `unittest` for tests.

A typical file-local helper, from `compiler/driver/toolchain/toolchain.c`:

```c
static bool runtime_directory(const char *directory) {
    char *header = join(directory, "ruva_runtime.h");
    bool exists = header != NULL && rv_host_regular_file(header);
    free(header);
    return exists;
}
```

## Repository rules

### At most six files per directory

A directory may directly contain at most six project files. Subfolders do not count toward the limit, so when a folder fills up, group related files into a subfolder. That is why `compiler/backend/c` has `statements/`, `functions/` and `collections/`, and why `tests/runtime` has `collections/` and `io/`. The rule covers every folder, including tests, fixtures, scripts and the pages of this guide. `.DS_Store` and `Thumbs.db` are not counted, and generated folders such as `build/` and `__pycache__/` are skipped.

Check it yourself at any time:

```sh
python3 scripts/repository/check-layout.py
```

```text
Layout check passed: at most 6 project files per directory.
```

When a folder is over the limit, the check names it:

```text
Layout check failed: tests/lexer has 7 files; group them into subdirectories (limit 6).
```

### Git hooks

The hooks in `.githooks` run the repository checks for you. Turn them on once per clone:

```sh
git config core.hooksPath .githooks
```

`pre-commit` runs `scripts/repository/check-docs.py` and `scripts/repository/check-layout.py` before every commit. `pre-push` runs `check-docs.py --pre-push`, which also looks at the commits you are about to push, and then `check-layout.py`. `check-docs.py` makes sure the rules in `.gitignore` are intact and that no ignored file has been forced into a commit or a push. Leave the existing `.gitignore` rules as they are.

The hooks must pass. Do not skip them with `--no-verify`. The same checks run as the first step of `make test`, and `make check-repository` runs them on their own.

### Line endings and build output

`.gitattributes` stores text files with LF line endings and keeps every file under `tests/fixtures` byte for byte as you wrote it. Build output stays in `build/`, which Git ignores, so generated files such as the Flex scanner are never committed.

## Commit messages

Write one short, plain, lowercase line that says what the commit changes. No prefixes such as `feat:` and no full stop at the end. Keep each commit to one logical change.

```text
add fixture for a during loop that counts down
clarify the error for pop on an empty array
add a lexer test for the arrow token
document the parser-probe flags
```

If a change needs a longer explanation, put it in the pull request description.

## Pull request checklist

Before you open a pull request, go through this list:

1. `make test` passes on your machine.
2. If you changed C code that allocates, frees or indexes memory, `make sanitize` passes as well.
3. New behavior or a fixed bug comes with a test that fails without your change.
4. New `.c` files are listed in the `Makefile`, and no directory has more than six files.
5. The code builds without warnings. If you can, try both GCC and Clang.
6. If users can see the change, the README or this guide is updated to match.
7. The change stays inside the 1.0 language.
8. The description says what changed, why, and how you tested it, and links the issue if there is one.

Small pull requests are reviewed faster. One fix or one improvement per pull request is ideal.

## Growing the language

Ruva is at version 1.0 and it is still growing. Nothing is out of scope. New syntax, new keywords, new built-in functions and new types are all welcome, along with improvements to the compiler, the runtime, error messages, tests, documentation, examples, platform support and the build.

A language change touches every stage, from the lexer and parser to the type checker, the C generator, the runtime, the tests and these docs. Before you start one, open an issue that shows the new syntax with a small example program and what it should print. Agreeing on the design first saves you from rewriting a large pull request.

If `ruvac` rejects a program that the [syntax reference](../reference/syntax.md) says is valid, or accepts one it says is invalid, that is a bug, and a fix is always welcome.

## Good first contributions

- Add a fixture for a case nobody has covered. Pick a page of the language guide, write a small program that uses the feature in an unusual way and check whether `tests/fixtures/native` already has it. [Testing](testing.md#adding-a-fixture-step-by-step) shows how to add one.
- Test the examples. Only `examples/modules` is compiled by the tests today. A test that builds every program in `examples/` and checks the output listed in the README would catch regressions early.
- Make an error message clearer. Find where it comes from, for example with `grep -rn '"unknown name"' compiler`, improve the wording and update the fixtures that expect the old text. The [errors page](../programs/errors.md) shows how messages look to users.
- Add a C unit test for an edge case in the lexer or parser that the suites do not check yet, modeled on the tests around it.
- Build with a different compiler or Linux distribution and report what happens, or run `make windows-test` on real Windows hardware and share `build/windows/verification.log`.
- Read one stage of the compiler with the [architecture tour](architecture.md) open, and fix whatever in this guide confused you.
