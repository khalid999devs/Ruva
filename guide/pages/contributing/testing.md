# Testing

Ruva has four kinds of tests: C unit suites, program fixtures, Python tests and repository checks. `make test` runs all of them, which today means 7 C suites, 177 fixtures and 78 Python tests in about two minutes. Every change should leave `make test` passing, and most changes should add a test of their own.

## The test suites

The C unit suites call compiler and runtime functions directly. Each one builds into `build/debug/tests/<suite>/`, prints a short line such as `Parser tests passed.` on success and stops at the first failed check with `file:line: check failed: expression`.

| Target | Program | What it covers |
| --- | --- | --- |
| `make test-foundation` | `foundation-tests` | Allocators, arenas, vectors and source loading |
| `make test-lexer` | `lexer-tests` | Tokens, text literals and lexer errors |
| `make test-parser` | `parser-tests` | Expressions, declarations, statements, ambiguous headers and parse errors |
| `make test-semantic` | `semantic-tests` | Expressions, records, ownership, names, modules and check errors |
| `make test-runtime` | `runtime-tests` | Runtime memory, text, numbers, arrays, input and files |
| `make test-backend` | `backend-tests` | C generation, including allocation and write failures |
| `make test-host` | `host-tests` | Host helpers for numbers and files, and the C compiler command line |

Program fixtures run a whole program and compare everything it does with expected files. They live in `tests/fixtures`, one folder per case.

| Fixture folder | Program under test | Target |
| --- | --- | --- |
| `tests/fixtures/foundation` | `fixture-probe` | `make test-fixtures` |
| `tests/fixtures/lexer` | `lexer-probe` | `make test-lexer-fixtures` |
| `tests/fixtures/parser` | `parser-probe` | `make test-parser-fixtures` |
| `tests/fixtures/semantic` | `semantic-probe` | `make test-semantic-fixtures` |
| `tests/fixtures/runtime` | `runtime-probe` | `make test-runtime-fixtures` |
| `tests/fixtures/native` | `ruvac`, through `scripts/testing/native.py` | `make test-native` |

The Python tests use only the standard library.

- `make test-integration` runs `tests/integration`. These tests drive the built `ruvac` end to end: paths with spaces and Unicode, repeatable `--emit-c` output, modules, cleanup after allocation failures inside generated programs, input and files.
- `make test-python` runs `tests/testing`, which tests the fixture runner and the Windows scripts, and `tests/repository`, which tests the Git hooks, the layout check, the `.gitattributes` rules and the Makefile.
- `tests/windows` runs only on Windows, through `make windows-test`.

The repository checks run first in `make test`, through `make check-repository`. [Workflow](workflow.md#repository-rules) explains them.

## How fixtures work

`scripts/testing/fixtures.py --program PROGRAM PATH...` finds every `case.json` under the paths you give it. For each case it creates a fresh temporary folder, copies in the files listed in `inputs`, runs the program there with `args` and feeds it `stdin`. Then it compares the exit code, standard output, standard error and any listed output files with the expected files, byte for byte.

A `case.json` is a JSON object with these fields, all optional:

| Field | Type | Meaning | Default |
| --- | --- | --- | --- |
| `args` | list of strings | Arguments passed to the program | `[]` |
| `inputs` | list of paths | Fixture files copied into the run folder, keeping subfolders | `[]` |
| `stdin` | path | File fed to standard input | nothing |
| `stdout` | path | Expected standard output | must be empty |
| `stderr` | path | Expected standard error | must be empty |
| `exit` | integer | Expected exit code | `0` |
| `files` | object | Maps a file the program writes in the run folder to the expected file in the fixture | `{}` |
| `timeout` | number | Seconds before the run counts as failed | `5` |

Unknown fields are an error, and every path must stay inside the fixture folder. Only `inputs` are copied, so the program never sees the expected files. `.gitattributes` tells Git never to touch line endings under `tests/fixtures`, so the bytes you commit are the bytes the runner compares.

!!! note
    Leaving out `stdout` or `stderr` does not mean "ignore it". It means the program must print nothing on that stream.

For native fixtures, the program is `native.py`. It compiles `input.ruv` with the compiler named in `RUVA_TEST_COMPILER`, runs the result and passes its output through. It also strips the run folder from error messages, so they read `input.ruv:1:17: error: ...` instead of a long temporary path. If compilation fails, the compiler's own error and exit code are what get compared, so native fixtures can test compile errors too.

Here is `tests/fixtures/native/divide-min`, a real fixture for a runtime error. The folder holds three files:

```text
tests/fixtures/native/divide-min/
  case.json
  input.ruv
  stderr.txt
```

`input.ruv` is the single line `launch() { show(-2147483648 / -1); }`. `stderr.txt` holds the expected error, `input.ruv:1:17: error: integer overflow`, followed by a newline. `case.json` ties them together:

```json
{
  "args": ["input.ruv"],
  "inputs": ["input.ruv"],
  "stderr": "stderr.txt",
  "exit": 1,
  "timeout": 15
}
```

## Adding a fixture step by step

This example adds `tests/fixtures/native/during-countdown`, a program that counts down with a `during` loop.

1. Pick the fixture set. Use `native` for anything a user would see from `ruvac` and the program: output, runtime errors and compile errors. Use `semantic`, `parser` or `lexer` when you want to check one stage's output in detail.

2. Create the folder and write `input.ruv`. Name the folder with a few lowercase words joined by hyphens that describe the behavior.

    ```ruva
    launch() {
        vary count = 3;
        during count > 0 {
            show(count);
            count = count - 1;
        }
        show("done");
    }
    ```

3. Write `case.json` next to it. Native fixtures usually set a longer timeout because they run a C compiler.

    ```json
    {
      "args": ["input.ruv"],
      "inputs": ["input.ruv"],
      "stdout": "stdout.txt",
      "timeout": 15
    }
    ```

4. Build and run the program by hand, and read the output carefully. It should print `3`, `2`, `1` and `done` on separate lines. Only when you are sure it is right, save it as the expected output. Building into `build/` keeps stray files out of the fixture folder.

    ```sh
    build/debug/bin/ruvac tests/fixtures/native/during-countdown/input.ruv -o build/countdown
    build/countdown
    build/countdown > tests/fixtures/native/during-countdown/stdout.txt
    ```

5. Run just this fixture.

    ```sh
    RUVA_TEST_COMPILER="$PWD/build/debug/bin/ruvac" python3 scripts/testing/fixtures.py \
        --program scripts/testing/native.py tests/fixtures/native/during-countdown
    ```

    ```text
    PASS /path/to/Ruva/tests/fixtures/native/during-countdown
    1/1 fixtures passed.
    ```

6. Make sure the test can fail. Delete the `done` line from `stdout.txt`, run it again and check that you get a `FAIL` with a clear difference, then put the line back.

    ```text
    FAIL /path/to/Ruva/tests/fixtures/native/during-countdown
      stdout: expected b'3\n2\n1\n', got b'3\n2\n1\ndone\n'
    0/1 fixtures passed.
    ```

7. Run `make test-native`, then `make test`.

A fixture for an error looks the same, with `stderr` and `exit` instead of `stdout`. For this `input.ruv`, `stderr.txt` holds `input.ruv:3:10: error: division by zero` and `case.json` sets `"stderr": "stderr.txt"` and `"exit": 1`.

```ruva
launch() {
    keep zero = 0;
    show(10 / zero);
}
```

A fixture folder follows the same six-file limit as every other folder. If a case needs more files, put extra modules in a subfolder, as `tests/fixtures/semantic/imports/parts/item.ruv` does.

## Running one suite or one fixture

```sh
make test-parser                                   # one C suite, rebuilt if needed
build/debug/tests/parser/parser-tests              # the same suite, run directly
python3 scripts/testing/fixtures.py --program build/debug/tests/lexer/lexer-probe tests/fixtures/lexer
python3 scripts/testing/fixtures.py --program build/debug/tests/lexer/lexer-probe tests/fixtures/lexer/literals
python3 -m unittest discover -s tests/integration -p test_driver.py -v
python3 -m unittest discover -s tests/integration -p test_driver.py -k emit_c -v
make check-repository
```

You can pass a fixture folder, a `case.json` file or a parent folder, and as many as you like. Native fixtures need `RUVA_TEST_COMPILER` as shown in step 5. Without it, `native.py` fails with `native fixture requires a compiler and one source`. The integration tests use `build/debug/bin/ruvac` unless `RUVA_TEST_COMPILER` points somewhere else.

## Adding a C unit test

Each suite is a set of `static bool` functions that use the `CHECK` macro from `tests/foundation/test.h`. A suite file chains its functions with `&&` in one exported function, which the suite's `main.c` calls. This test would fit in the lexer suite:

```c
static bool arrow_after_parameters(void) {
    const char input[] = "craft f() -> int";
    RvSource *source = rv_source_from_bytes(rv_allocator_system(), "arrow.ruv",
                                            input, sizeof(input) - 1, NULL);
    CHECK(source != NULL);

    RvTokenStream stream;
    CHECK(rv_lex_source(source, rv_allocator_system(), &stream, NULL));
    CHECK(rv_token_stream_count(&stream) == 7);
    CHECK(rv_token_stream_at(&stream, 4)->kind == RV_TOKEN_ARROW);

    rv_token_stream_destroy(&stream);
    rv_source_destroy(source);
    return true;
}
```

Add it to an existing file when it fits there. If you create a new `.c` file, add it to the matching `*_TEST_SOURCES` list in the `Makefile`, declare its exported function in the suite's `test.h` and call that function from the suite's `main.c`. Most test folders already hold six files, so a new file usually goes in a subfolder such as `tests/parser/support/`.

## Sanitizer runs

`make sanitize` builds everything again in `build/sanitize` with AddressSanitizer and UBSan, then runs the C suites, all fixtures and the integration tests. Programs built by `ruvac` during those tests are sanitized too. Run it whenever you touch code that allocates, frees or indexes memory.

To run one suite under sanitizers, pass the same flags that `make sanitize` uses:

```sh
make BUILD_DIR=build/sanitize \
    CFLAGS="-O1 -g -fno-omit-frame-pointer -fsanitize=address,undefined -fno-sanitize-recover=all" \
    LDFLAGS="-fsanitize=address,undefined" test-parser
```

Add `NATIVE_SANITIZE=1` to that command with `test-native` or `test-integration` to sanitize the compiled Ruva programs as well. Sanitizers are not available on the Windows GCC target.
