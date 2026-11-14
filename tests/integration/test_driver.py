import json
import os
import subprocess
import unittest

from support import COMPILER, ROOT, RUNTIME_SOURCES, SANITIZE, CompilerTestCase


class DriverTests(CompilerTestCase):
    def test_native_paths_spaces_unicode_and_shell_characters(self):
        name = "code $literal; বাংলা.ruv"
        self.source.rename(self.directory / name)
        executable = self.compile(name, "output; touch unexpected")
        result = subprocess.run([str(executable)], capture_output=True, timeout=10)
        self.assertEqual((result.returncode, result.stdout, result.stderr), (0, b"native\n", b""))
        self.assertFalse((self.directory / "unexpected").exists())
        self.assert_no_temporary_outputs()

    def test_emit_c_is_deterministic_and_compiles_independently(self):
        for output in ("first.c", "second.c"):
            result = self.invoke("--emit-c", "input.ruv", "-o", output)
            self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual((self.directory / "first.c").read_bytes(),
                         (self.directory / "second.c").read_bytes())
        arguments = [os.environ.get("CC", "cc"), "-std=c11", "-Wall", "-Wextra",
                     "-Wpedantic", "-Werror", "-I", str(ROOT / "runtime"), "first.c"]
        arguments.extend(str(ROOT / "runtime" / name) for name in RUNTIME_SOURCES)
        arguments.extend(["-o", "independent"])
        if SANITIZE:
            arguments.extend(["-fsanitize=address,undefined", "-fno-sanitize-recover=all"])
        result = subprocess.run(arguments, cwd=self.directory, capture_output=True, timeout=30)
        self.assertEqual(result.returncode, 0, result.stderr)
        result = subprocess.run([str(self.directory / "independent")], capture_output=True, timeout=10)
        self.assertEqual(result.stdout, b"native\n")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assert_no_temporary_outputs()

    def test_full_language_check_does_not_invoke_c_compiler(self):
        self.source.write_text(
            "object Item { value: int; } craft f(item: Item) -> int { give item.value; }"
            "launch() { vary values = [1]; walk value in values { show(value); }"
            'show(f(Item { value: 2 }), ask(int)); }'
        )
        result = self.invoke("--check", "input.ruv", environment=self.mock_compiler())
        self.assertEqual((result.returncode, result.stdout, result.stderr), (0, b"", b""))
        self.assertFalse((self.directory / "arguments.json").exists())
        self.assertFalse((self.directory / "a.out").exists())

    def test_invalid_source_preserves_outputs(self):
        output = self.directory / "existing"
        output.write_bytes(b"keep me")
        for source, message in [
            ("launch() { show(missing); }", b"unknown name"),
            ('launch() { give; during false { toText("invalid"); } }', b"conversion does not accept"),
            ('launch() { fileOpen(1, "read"); }', b"argument type mismatch"),
        ]:
            self.source.write_text(source)
            result = self.invoke("input.ruv", "-o", "existing", environment=self.mock_compiler())
            self.assertNotEqual(result.returncode, 0)
            self.assertIn(message, result.stderr)
            self.assertEqual(output.read_bytes(), b"keep me")
            self.assertFalse((self.directory / "arguments.json").exists())
            self.assert_no_temporary_outputs()

    def test_compiler_failure_is_not_a_shell_and_preserves_output(self):
        (self.directory / "existing").write_bytes(b"keep me")
        result = self.invoke("input.ruv", "-o", "existing", environment=self.mock_compiler())
        self.assertNotEqual(result.returncode, 0)
        self.assertIn(b"C compiler failed", result.stderr)
        arguments = json.loads((self.directory / "arguments.json").read_text())
        self.assertIn("-std=c11", arguments)
        self.assertIn("-fno-fast-math", arguments)
        self.assertEqual(len([value for value in arguments if value.endswith(".c")]), len(RUNTIME_SOURCES) + 1)
        self.assertEqual((self.directory / "existing").read_bytes(), b"keep me")
        self.assert_no_temporary_outputs()
        environment = self.environment.copy()
        environment["CC"] = "cc; touch injected"
        result = self.invoke("input.ruv", environment=environment)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn(b"cannot start C compiler", result.stderr)
        self.assertFalse((self.directory / "injected").exists())
        self.assert_no_temporary_outputs()

    def test_signaled_or_empty_successful_compiler_is_failure(self):
        for environment, message in [
            (self.mock_compiler(signal=True), b"C compiler failed"),
            (self.mock_compiler(exit_code=0), b"did not produce an executable"),
        ]:
            result = self.invoke("input.ruv", "-o", "program", environment=environment)
            self.assertNotEqual(result.returncode, 0)
            self.assertIn(message, result.stderr)
            self.assertFalse((self.directory / "program").exists())
            self.assert_no_temporary_outputs()

    def test_sources_hardlinks_and_nonregular_outputs_are_protected(self):
        original = self.source.read_bytes()
        os.link(self.source, self.directory / "alias")
        for output in ("input.ruv", "alias"):
            result = self.invoke("--emit-c", "input.ruv", "-o", output)
            self.assertNotEqual(result.returncode, 0)
            self.assertIn(b"overwrite a source file", result.stderr)
            self.assertEqual(self.source.read_bytes(), original)
        (self.directory / "folder").mkdir()
        (self.directory / "symlink").symlink_to("input.ruv")
        for output in ("folder", "symlink", "missing/output"):
            result = self.invoke("--emit-c", "input.ruv", "-o", output)
            self.assertNotEqual(result.returncode, 0)
            self.assertEqual(self.source.read_bytes(), original)
            self.assert_no_temporary_outputs()

    def test_missing_runtime_and_workspace_preserve_outputs(self):
        for key, value, message in [
            ("RUVA_RUNTIME_ROOT", str(self.directory / "missing"), b"cannot locate runtime"),
            ("TMPDIR", str(self.directory / "missing"), b"cannot create compiler workspace"),
        ]:
            environment = self.environment.copy()
            environment[key] = value
            result = self.invoke("input.ruv", "-o", "program", environment=environment)
            self.assertNotEqual(result.returncode, 0)
            self.assertIn(message, result.stderr)
            self.assertFalse((self.directory / "program").exists())
            self.assert_no_temporary_outputs()

    def test_driver_invoked_through_path_finds_runtime(self):
        environment = self.environment.copy()
        environment["PATH"] = str(COMPILER.parent) + os.pathsep + environment.get("PATH", "")
        arguments = ["ruvac", "input.ruv", "-o", "program"]
        if SANITIZE:
            arguments.append("--sanitize")
        result = subprocess.run(arguments, cwd=self.directory, env=environment,
                                capture_output=True, timeout=30)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertTrue((self.directory / "program").exists())
        self.assert_no_temporary_outputs()

    def test_missing_source_options_defaults_and_dash_paths(self):
        result = self.invoke("--help")
        self.assertEqual(result.returncode, 0)
        self.assertIn(b"usage: ruvac", result.stdout)
        for arguments in [(), ("--unknown",), ("input.ruv", "extra"),
                          ("--check", "--emit-c", "input.ruv"), ("input.ruv", "-o"),
                          ("--check", "input.ruv", "-o", "out"),
                          ("--emit-c", "--sanitize", "input.ruv"), ("missing.ruv",)]:
            result = self.invoke(*arguments)
            self.assertNotEqual(result.returncode, 0, arguments)
            self.assertIn(b"error:", result.stderr)
        self.source.rename(self.directory / "-input.ruv")
        result = self.invoke("--emit-c", "--", "-input.ruv")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertTrue((self.directory / "out.c").exists())
        self.assert_no_temporary_outputs()

    def test_normal_and_early_text_exits_reclaim_before_final_cleanup(self):
        programs = [
            'launch() { vary text_value = "a"; keep saved = text_value; '
            'text_value = text_value + "b"; { keep copy = saved; show(copy); } }',
            'launch() { keep outer = "a"; { keep inner = outer; give; } }',
        ]
        for source in programs:
            self.source.write_text(source)
            result = self.invoke("--emit-c", "input.ruv", "-o", "lifetime.c")
            self.assertEqual(result.returncode, 0, result.stderr)
            generated = self.directory / "lifetime.c"
            code = generated.read_text().replace(
                "rv_launch(&runtime);",
                "rv_launch(&runtime);\n    if (runtime.live_allocations != 0) return 91;"
            )
            generated.write_text(code)
            arguments = [os.environ.get("CC", "cc"), "-std=c11", "-Wall", "-Wextra",
                         "-Wpedantic", "-Werror", "-I", str(ROOT / "runtime"), "lifetime.c"]
            arguments.extend(str(ROOT / "runtime" / name) for name in RUNTIME_SOURCES)
            arguments.extend(["-o", "lifetime"])
            if SANITIZE:
                arguments.extend(["-fsanitize=address,undefined", "-fno-sanitize-recover=all"])
            result = subprocess.run(arguments, cwd=self.directory, capture_output=True, timeout=30)
            self.assertEqual(result.returncode, 0, result.stderr)
            result = subprocess.run([str(self.directory / "lifetime")], capture_output=True, timeout=10)
            self.assertEqual(result.returncode, 0, result.stderr)

    def test_broken_output_pipe_reports_error(self):
        executable = self.compile()
        read_end, write_end = os.pipe()
        os.close(read_end)
        try:
            result = subprocess.run([str(executable)], stdout=write_end, stderr=subprocess.PIPE, timeout=10)
        finally:
            os.close(write_end)
        self.assertEqual(result.returncode, 1)
        self.assertIn(b"input.ruv:1:12: error: output failed", result.stderr)


if __name__ == "__main__":
    unittest.main()
