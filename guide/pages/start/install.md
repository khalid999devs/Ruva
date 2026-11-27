# Install Ruva

You build `ruvac` from source once, then install it so you can use it from any folder, the same way you use `gcc`.

## What you need

| Tool | Why |
| --- | --- |
| A C11 compiler (GCC or Clang) | Builds `ruvac`, and later builds every Ruva program |
| Flex | Generates the Ruva lexer during the build |
| Make | Runs the build |
| Git | Downloads the source |
| Python 3.9 or later | Only for running the test suite |

=== "macOS"

    The Xcode command line tools include Clang, Make, Flex and Git.

    ```sh
    xcode-select --install
    ```

=== "Debian and Ubuntu"

    ```sh
    sudo apt install build-essential flex git python3
    ```

=== "Fedora"

    ```sh
    sudo dnf install gcc make flex git python3
    ```

## Build the compiler

```sh
git clone https://github.com/khalid999devs/Ruva.git
cd Ruva
make
```

The build takes a few seconds. The compiler ends up at `build/debug/bin/ruvac`.

## Install it

Link the compiler into a folder that is already on your path:

```sh
sudo ln -s "$PWD/build/debug/bin/ruvac" /usr/local/bin/ruvac
```

Check that your shell finds it:

```sh
which ruvac
```

```text
/usr/local/bin/ruvac
```

Running `ruvac` with nothing else prints its usage line:

```text
ruvac: error: a source path is required
usage: ruvac [--check | --emit-c] [--sanitize] source [-o output]
```

!!! warning "Keep the Ruva folder"
    `ruvac` uses the runtime sources inside the folder you cloned, and it finds them by following the link back to that folder. If you move or delete the clone, link it again from the new place.

### Without sudo

If you would rather not touch `/usr/local/bin`, add the build folder to your path instead. Put this line in `~/.zshrc` or `~/.bashrc`, using the folder where you cloned Ruva:

```sh
export PATH="$HOME/Ruva/build/debug/bin:$PATH"
```

Open a new terminal and `ruvac` works from anywhere.

## Update

```sh
cd Ruva
git pull
make
```

The link keeps pointing at the new build, so there is nothing else to do.

## Uninstall

```sh
sudo rm /usr/local/bin/ruvac
```

Then delete the cloned folder if you no longer need it.

## Windows

A Windows x64 build with MSYS2 GCC is prepared but has not been tested on a real Windows machine yet. The steps are in [Building](../contributing/building.md).

Next: [Your first program](first-program.md)
