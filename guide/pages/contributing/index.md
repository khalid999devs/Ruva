# Contributing to Ruva

Ruva has a real compiler behind it. `ruvac` reads a `.ruv` file, splits it into tokens, builds a syntax tree, follows its imports, checks names and types, writes C and asks GCC or Clang to turn that C into a native program. Every one of those steps lives in this repository, written in plain C11.

The whole compiler is around 8,000 lines, and the runtime that ships with every program is about 1,000 more. That is small enough to read in a few evenings and complete enough to show how a working compiler fits together. Contributing is a good way to learn it, and you do not need to know anything about compilers before you start.

## Why contribute

- You get to see a compiler from end to end, from bytes in a file to a running executable, without getting lost in a huge codebase.
- Each stage has its own folder, its own tests and, for the front end, a small tool that prints exactly what the stage produced.
- The tests are fast and strict. You find out within minutes whether a change broke something.
- Small changes count. One new test or one clearer error message is a real improvement and a good first pull request.

## What kinds of contributions help

| Kind | What it looks like |
| --- | --- |
| Tests | A program fixture for a case nobody covered yet, or a C unit test for an edge case in one stage |
| Error messages | A message that explains the problem better or points at a more useful line and column |
| Documentation | Fixing a confusing page in this guide, or adding the explanation you wished you had |
| Examples | Short, readable programs in `examples/` that show a feature in use |
| Bug fixes | A program that crashes `ruvac`, produces C that fails to build, or prints the wrong result |
| Platform support | Building with other compilers and Linux distributions, or running the Windows build on real hardware |
| Language features | New syntax, keywords, built-in functions or types, discussed in an issue first |

Nothing is out of scope. Ruva is at version 1.0 and the language itself can grow as it needs to. [Growing the language](workflow.md#growing-the-language) explains how to propose a change.

## From clone to first pull request

You need Git, a C11 compiler (GCC or Clang), Flex, Make and Python 3.9 or later. [Building](building.md#requirements) lists what to install on each platform.

1. Fork the repository. Open [github.com/khalid999devs/Ruva](https://github.com/khalid999devs/Ruva) and press **Fork**.

2. Clone your fork and add the main repository as `upstream`, so you can pull in new changes later.

    ```sh
    git clone https://github.com/YOUR-USERNAME/Ruva.git
    cd Ruva
    git remote add upstream https://github.com/khalid999devs/Ruva.git
    ```

3. Turn on the project's Git hooks. They run the repository checks before every commit and push.

    ```sh
    git config core.hooksPath .githooks
    ```

4. Build everything. The compiler lands in `build/debug/bin/ruvac`, next to the test programs and inspection tools.

    ```sh
    make
    ```

5. Run the full test suite once before you change anything, so you know your starting point is clean. It takes about two minutes.

    ```sh
    make test
    ```

6. Create a branch for your work.

    ```sh
    git switch -c during-countdown-fixture
    ```

7. Make your change. [Architecture](architecture.md) tells you which folder holds which stage.

8. Add a test that fails without your change and passes with it. [Testing](testing.md#adding-a-fixture-step-by-step) walks through adding a program fixture, which takes a few minutes.

9. Run `make test` again. When it passes, commit with a short lowercase message and push the branch to your fork.

    ```sh
    git add tests/fixtures/native/during-countdown
    git commit -m "add fixture for a during loop that counts down"
    git push -u origin during-countdown-fixture
    ```

10. Open a pull request from your branch on GitHub. Say what you changed, why, and how you tested it. The [pull request checklist](workflow.md#pull-request-checklist) covers the rest.

## Asking questions

Use [GitHub issues](https://github.com/khalid999devs/Ruva/issues) for questions as well as bugs. "Where does the compiler decide this?" and "Is this a bug or am I reading the language wrong?" are both fine things to ask.

When you report a problem, include the `.ruv` program, the exact command you ran and the full output. If you want to work on something bigger than a small fix, open an issue first and describe your plan. That way you can agree on the approach before you spend an evening on it.

## Where to read next

- [Architecture](architecture.md) is a tour of the compiler, stage by stage, with a small program followed all the way through.
- [Building](building.md) covers `make` targets, build options, sanitizers, Windows and the inspection tools.
- [Testing](testing.md) explains every test suite and shows how to add a fixture.
- [Workflow](workflow.md) has the code style, the repository rules, commit messages and the pull request checklist.

If you have not used Ruva yet, start with [installing it](../start/install.md) and [using `ruvac`](../start/compiler.md). The [syntax reference](../reference/syntax.md) and the [built-in functions](../reference/builtins.md) are handy while you read the compiler source.
