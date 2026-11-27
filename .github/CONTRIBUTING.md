# Contributing to Ruva

Thanks for your interest. Ruva is a learning project, and contributions of every size are welcome, from fixing a typo to improving the type checker.

The full guide lives on the documentation site:

- [Start here](https://khalid999devs.github.io/Ruva/contributing/) for the path from your first build to your first pull request
- [Architecture](https://khalid999devs.github.io/Ruva/contributing/architecture/) for a tour of the compiler
- [Testing](https://khalid999devs.github.io/Ruva/contributing/testing/) for how to run and add tests
- [Workflow](https://khalid999devs.github.io/Ruva/contributing/workflow/) for code style, repository rules and pull requests

The same pages are in [`guide/pages/contributing`](../guide/pages/contributing) if you prefer to read them on GitHub.

The short version:

```sh
git clone https://github.com/khalid999devs/Ruva.git
cd Ruva
git config core.hooksPath .githooks
make
make test
```

Make your change, add a test for it, make sure `make test` passes and open a pull request. If you are not sure where to start, open an issue and ask.
