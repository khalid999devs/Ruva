# Your first program

This page assumes `ruvac` is installed. If it is not, start with [Install Ruva](install.md).

## Hello

Make a folder anywhere and create a file called `main.ruv`:

```sh
mkdir ~/hello
cd ~/hello
```

```ruva
launch() {
    show("Hello from Ruva");
}
```

Compile it and run the result:

```sh
ruvac main.ruv
./a.out
```

```text
Hello from Ruva
```

`ruvac` printed nothing because the compile worked. It wrote an executable called `a.out` next to your source file.

## What the program means

- `launch()` is where every Ruva program starts. Each program has exactly one.
- The braces hold the statements that run, in order.
- `show` prints its values on one line and ends the line.
- Every statement ends with a semicolon.

## Name the output

Use `-o` to choose the executable name:

```sh
ruvac main.ruv -o hello
./hello
```

## A slightly bigger program

Replace `main.ruv` with this:

```ruva
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

```text
7 squared is 49
```

`craft` defines a function. `square` takes an `int` called `n` and gives back an `int`. `show` puts a space between the values it prints.

## When something is wrong

Change `square(7)` to `square("seven")` and compile again. `ruvac` stops and tells you where the problem is:

```text
main.ruv:6:33: error: function argument type mismatch
```

The numbers are the line and the column. The real message starts with the full path of the file. No executable is written until the program is correct.

## Run the examples

The repository has more programs in `examples/`:

```sh
cd Ruva/examples/functions
ruvac main.ruv && ./a.out
```

```text
120 rururu
```

Next: [Using ruvac](compiler.md), or jump into the language with [Program structure](../basics/program.md).
