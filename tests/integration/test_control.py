import unittest

from observation import ObservedCompilerTestCase


class ControlTests(ObservedCompilerTestCase):
    def test_normal_and_nested_returns_release_before_fallback(self):
        programs = [
            'craft identity(value: text, depth: int) -> text { keep local = value; '
            'check depth == 0 { choose local { when "kept" => { give local; } '
            'otherwise => { give local + "!"; } } } otherwise { give identity(local, depth - 1); } } '
            'launch() { vary value = "kept"; value = identity(value, 6); '
            'choose value {} show(value); }',
            'craft select(value: text) -> text { keep outer = value + "!"; '
            'walk i from 0 until 2 { keep copy = outer; during true { choose copy { '
            'when "ok!" => { keep result = copy + " selected"; give result; } '
            'otherwise => { give copy; } } } } give "empty"; } '
            'launch() { show(select("ok"), select("other")); }',
            'launch() { keep outer = "kept"; during true { choose outer { '
            'when "kept" => { keep copy = outer; give; } otherwise => {} } } }',
        ]
        for source in programs:
            with self.subTest(source=source):
                result = self.run_observed(source)
                self.assertEqual(result.returncode, 0, result.stderr)

    def test_repeated_loop_and_function_text_has_bounded_live_allocations(self):
        result = self.run_observed(
            'craft identity(value: text) -> text { keep copy = value; give copy; } '
            'launch() { vary result = ""; walk i from 0 until 10000 { keep before = result; '
            'choose before { when "" => { result = identity(before) + "x"; } '
            'otherwise => { result = identity(before) + "x"; } } } '
            'show(result == ""); }', peak_limit=8
        )
        self.assertEqual((result.returncode, result.stdout, result.stderr), (0, b"false\n", b""))

    def test_recursive_failure_cleans_all_frames_and_call_arguments(self):
        result = self.run_observed(
            'craft fail(value: text, depth: int) -> int { keep local = value + "!"; '
            'check depth == 0 { give 1 / 0; } otherwise { give fail(local, depth - 1); } } '
            'launch() { keep outer = "live"; show(fail(outer, 8)); }'
        )
        self.assertEqual(result.returncode, 1)
        self.assertEqual(result.stdout, b"")
        self.assertIn(b"error: division by zero", result.stderr)

    def test_invalid_function_and_loop_programs_fail_before_cc(self):
        programs = [
            ('craft f(x: int) -> void { x = 1; } launch() {}', b"read-only"),
            ('craft f(x: int) -> void {} launch() { f(); }', b"argument count"),
            ('craft f(x: int) -> void {} launch() { f(1.0); }', b"argument type"),
            ('craft f(x: bool) -> int { check x { give 1; } } launch() {}', b"every path"),
            ('launch() { walk i from 0 until 1 { i = 2; } }', b"read-only"),
            ('launch() { choose 1 { when 01 => {} when 1 => {} } }', b"duplicate case"),
        ]
        for source, message in programs:
            self.source.write_text(source)
            result = self.invoke("input.ruv", environment=self.mock_compiler())
            self.assertNotEqual(result.returncode, 0)
            self.assertIn(message, result.stderr)
            self.assertFalse((self.directory / "arguments.json").exists())
            self.assertFalse((self.directory / "a.out").exists())
            self.assert_no_temporary_outputs()

    def test_unused_recursion_and_control_emission_remain_deterministic(self):
        source = (
            'craft forever() -> void { forever(); } '
            'craft identity(value: text) -> text { give value; } '
            'launch() { choose identity("a") { when "a" => { show("selected"); } '
            'otherwise => {} } }'
        )
        self.source.write_text(source)
        for name in ("first.c", "second.c"):
            result = self.invoke("--emit-c", "input.ruv", "-o", name)
            self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual((self.directory / "first.c").read_bytes(),
                         (self.directory / "second.c").read_bytes())
        result = self.run_observed(source)
        self.assertEqual((result.returncode, result.stdout, result.stderr), (0, b"selected\n", b""))


if __name__ == "__main__":
    unittest.main()
