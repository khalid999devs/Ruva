# Compiler architecture

`ruvac` is a pipeline. Each stage takes what the stage before it produced, adds one kind of knowledge and passes the result on. This page goes through the stages in order, names the folder, data structures and entry function for each, and then follows a three-line program all the way to a running executable.

| Stage | Folder | Takes in | Produces | Entry function |
| --- | --- | --- | --- | --- |
| Source loading | `compiler/common` | a file path | `RvSource` | `rv_source_load` |
| Lexer | `compiler/frontend/lexer` | `RvSource` | `RvTokenStream` | `rv_lex_source` |
| Parser | `compiler/frontend/parser` and `ast` | `RvTokenStream` | `RvAstModule` | `rv_parse_module` |
| Module graph | `compiler/analysis/modules` | the entry path | `RvModuleGraph` | `rv_module_graph_load` |
| Checker | `compiler/analysis/semantics` | the entry path | `RvCheckedProgram` | `rv_check_program` |
| C backend | `compiler/backend/c` | `RvCheckedProgram` | one C file | `rv_c_emit` |
| Driver | `compiler/driver` | command line | executable or C file | `run` in `main.c` |
| Runtime | `runtime` | calls from the generated C | program behavior | `rv_runtime_init` |

## Shared foundations

`compiler/common` holds the pieces every stage uses.

- `memory/allocator.h` defines `RvAllocator`, a small struct of `allocate`, `resize` and `release` callbacks. Every stage takes one as a parameter. Tests pass in an allocator that fails on purpose to prove that each stage cleans up after any allocation failure. `rv_size_add` and `rv_size_multiply` do size arithmetic that cannot overflow silently.
- `memory/arena.h` defines `RvArena`, which hands out memory that is freed all at once. Tokens, tree nodes and checker data live in arenas. `memory/vector.h` defines `RvVector`, a growable array.
- `source/source.h` defines `RvSource`, a loaded file with its bytes and line table. `source/span.h` defines `RvSpan`, a byte range inside a source. Every token, tree node and error carries a span, which is how errors can name an exact line and column.
- `diagnostics/diagnostic.h` provides `rv_diagnostic_write`, which prints `path:line:column: error: message`.
- `host/` wraps file access, reading and formatting real numbers and standard streams, with `host/win32/` for Windows. `platform.h` refuses to build on a machine without 8-bit bytes or IEEE doubles, and refuses fast-math builds.

## Lexer

`scanner.l` is the Flex input. Make runs `flex` on it and writes the generated scanner to `build/debug/compiler/frontend/lexer/scanner.c`, so the generated file never lands in the source tree. Each Flex rule calls `rv_scan_emit` in `lexer.c`, which does the work Flex is not good at: checking UTF-8 and rejecting NUL bytes up front, decoding escapes in text literals and recording errors as an `RvLexError`. `token.c` maps identifiers to keywords with `rv_token_keyword`.

The entry point is `rv_lex_source(source, allocator, &stream, &error)`. It produces an `RvTokenStream`, which is an arena plus a vector of `RvToken`. Each token has a `kind` from `RvTokenKind`, a `span`, its raw `spelling` and, for text literals, the decoded `text`. The stream always ends with `RV_TOKEN_EOF`. Built-in functions such as `show` and `push` are ordinary identifiers at this stage.

## Parser and syntax tree

The parser is hand-written recursive descent. `parser.c` has the token helpers (`rv_parse_peek`, `rv_parse_take`, `rv_parse_match`, `rv_parse_expect`) and matches every bracket before parsing starts, so a missing `}` is reported where it opened. `declaration.c` handles the top level, `statement.c` handles statements and `expression.c` handles operators by precedence climbing in `parse_binary`. Shared parser state is the `RvParser` struct in `internal.h`. A nesting limit (`RV_PARSE_MAX_DEPTH`) and a work budget keep deeply nested or hostile input from overflowing the stack or running forever.

The entry point is `rv_parse_module(tokens, entry, allocator, &module, &error)`. The `entry` flag says whether this file must contain `launch()` or is an imported module that must not. The result is an `RvAstModule` with lists of `imports` and `declarations` plus the `launch` declaration. `ast/ast.h` defines the node types `RvAstDecl`, `RvAstStmt`, `RvAstExpr` and `RvAstType`, and `ast/dump.c` prints a tree with `rv_ast_dump`. Expression nodes have empty `type` and `symbol` fields that the checker fills in later.

One detail is worth a look. In `check flag {} { show(1); }`, the text `flag {}` could be a record value or a condition followed by an empty block, and syntax alone cannot tell. The parser keeps both readings as alternatives in an `RvAstSequence`, and the checker picks the one that fits once it knows what `flag` names (`select_sequence` in `semantics/statement.c`). `tests/parser/ambiguity.c` shows it in action.

## Module graph

`rv_module_graph_load(path, allocator, &graph, &error)` in `modules/loader.c` loads the entry file and follows each `bring` import relative to the file that imports it. For every file it loads the source, lexes it and parses it. Paths are made canonical, so one file reached through two imports is loaded once. A file that is imported again while it is still loading is reported as a `circular import`. The result is an `RvModuleGraph` holding one `RvModuleUnit` per file, each with its path, source, syntax tree and import edges.

## Checker

`rv_check_program(path, allocator, &program, &error)` in `semantics/check.c` is the front door for the whole front end. It builds the module graph and then runs three passes:

1. `rv_check_builtins` in `scope.c` puts `show`, `ask`, `push` and the other built-ins into a scope of their own.
2. `rv_check_declarations` in `declarations.c` collects every top-level name in every module, binds imports, and then resolves record fields, function signatures and constant types. Because every name is known before any body is checked, functions can call each other in any order.
3. `rv_check_bodies` checks each function body and `launch()`, using `statement.c` and the files in `expressions/`.

The data lives in a few small headers. `types/type.h` has `RvType`. `symbols/symbol.h` has `RvSymbol`, `RvSymbolKind` and the `RvBuiltin` list. `semantics/internal.h` has `RvScope` and the `RvChecker` state. Errors are an `RvCheckError` from `diagnostics/error.h`, with a kind such as `RV_CHECK_NAME` or `RV_CHECK_TYPE`, a span and a message. `rv_check_fail` keeps only the first error, so the compiler stops at the first problem. The loader converts lexer and parser errors into `RvCheckError` too, which means callers handle a single error type.

The result is an `RvCheckedProgram` with the module graph, every symbol and a `valid` flag.

## C backend

`rv_c_supported` in `validation.c` confirms that every construct can be emitted before any output file is touched. `rv_c_emit(output, program, allocator, &error)` in `emit.c` then writes one C file: an include of `ruva_runtime.h`, the record structs (`collections/record.c`), the functions (`functions/declaration.c`), a `static void rv_launch(RvRuntime *rt)` for the `launch()` body and a `main` that sets up the runtime. The emitter state is `RvCEmitter` in `internal.h`. Control flow lives in `statements/`, calls and built-ins in `functions/`, and arrays, records, walks and cleanup of owned values in `collections/`.

## Driver

`run` in `driver/main.c` parses the command line with `rv_options_parse` into an `RvOptions`, calls `rv_check_program` and then stops (`--check`), writes C (`--emit-c`) or builds a program with `rv_toolchain_build` from `toolchain/toolchain.c`. That function finds the runtime, writes the C into a temporary folder as `program.c` and runs the C compiler with strict warnings, `-fno-fast-math` and every runtime source file. Output goes to a temporary file next to the destination (`RvOutput` in `files/output.c`) and is renamed into place only on success, so a failed build never leaves a half-written file behind. Platform-specific process and file code is in `toolchain/posix.c` and `toolchain/windows.c`.

## Runtime

The runtime is plain C that is compiled together with every program. `runtime/ruva_runtime.h` is the whole interface the generated code uses, with `RvRuntime`, `RvText`, `RvArray`, `RvFile` and `RvLocation`. `core/` sets up and tears down the runtime, tracks allocations and does checked integer arithmetic such as `rv_int_add`. The other folders cover `text/`, `collections/`, `conversion/`, `io/` (keyboard input) and `files/`. A runtime error goes through `rv_panic`, which prints the Ruva file, line and column, frees what is still live and exits with status 1.

## Following a tiny program

Save this as `total.ruv` and compile it with `ruvac total.ruv`.

```ruva
launch() {
    keep total = 1 + 2 * 3;
    show(total);
}
```

`run` parses the options, picks `a.out` as the output name and calls `rv_check_program`. The loader reads the file and lexes it. The [inspection tools](building.md#inspection-tools) let you watch each step. The first lines of `lexer-probe total.ruv` show each token with its line and column:

```text
1:1 launch "launch"
1:7 ( "("
1:8 ) ")"
1:10 { "{"
2:5 keep "keep"
2:10 identifier "total"
2:16 = "="
2:18 integer "1"
```

The parser turns those tokens into a tree. In this part of the output of `parser-probe total.ruv`, `infer` means no type was written, and precedence has put the multiplication under the addition:

```text
(keep total infer
  (binary +
    (integer 1)
    (binary *
      (integer 2)
      (integer 3)
    )
  )
)
```

The checker finds one module with no imports. It gives `total` the type `int` from its initializer and resolves `show` to the built-in symbol. `semantic-probe --summary total.ruv` prints `modules=1 symbols=19`: 17 built-ins, `launch` and `total`.

The backend writes C. Here is the middle of `ruvac --emit-c total.ruv -o total.c`, with the source path shortened to `"..."` (the real file spells it out with octal escapes):

```c
int32_t rv_t_1 = INT32_C(1);
int32_t rv_t_2 = INT32_C(2);
int32_t rv_t_3 = INT32_C(3);
int32_t rv_t_4 = rv_int_multiply(rt, rv_t_2, rv_t_3, (RvLocation){"...", 2, 22});
int32_t rv_t_5 = rv_int_add(rt, rv_t_1, rv_t_4, (RvLocation){"...", 2, 18});
int32_t rv_v_19 = rv_t_5;
```

Every step gets its own numbered temporary, which keeps the order of evaluation explicit. The `*` and `+` became runtime calls that stop the program with an error on 32-bit overflow, and each call carries the Ruva line and column for that error. The variable is named after symbol 19.

Finally `rv_toolchain_build` compiles that C with the runtime sources, the driver renames the result to `a.out`, and running it prints `7`.

## Suggested reading order

1. `compiler/common/source/source.h` and `span.h`, to see how positions work.
2. `compiler/frontend/lexer/token.h`, `scanner.l` and `lexer.c`. Run `lexer-probe` on the files in `examples/`.
3. `compiler/frontend/ast/ast.h`, then `parser/internal.h`, `expression.c`, `statement.c` and `declaration.c`. Run `parser-probe` as you read.
4. `compiler/analysis/modules/module.h` and `loader.c`.
5. `compiler/analysis/semantics/check.c`, then `declarations.c`, `statement.c` and `expressions/expression.c`.
6. `compiler/backend/c/emit.h`, `emit.c` and `expression.c`, side by side with the output of `ruvac --emit-c`.
7. `compiler/driver/main.c` and `toolchain/toolchain.c`.
8. `runtime/ruva_runtime.h` and `runtime/core/numeric.c`.

Read the matching tests as you go, for example `tests/lexer/tokens.c` or `tests/parser/expressions.c`. They are short and show exactly what each function promises.
