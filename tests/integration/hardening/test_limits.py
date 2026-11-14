import subprocess
import unittest

from support import CompilerTestCase


class LimitTests(CompilerTestCase):
    def test_record_walk_expansion_is_bounded_and_preserves_output(self):
        fields = " ".join(f"field{index}: int;" for index in range(8))
        values = ", ".join(f"field{index}: {index}" for index in range(8))
        walks = " ".join(f"walk field{index} in item {{" for index in range(7))
        self.source.write_text(
            f"object Item {{ {fields} }} launch() {{ keep item = Item {{ {values} }}; "
            f"{walks} show(field0.key, field6.value); " + "}" * 8
        )
        output = self.directory / "existing.c"
        output.write_bytes(b"preserved\n")
        environment = self.mock_compiler()
        for arguments in [("--emit-c", "input.ruv", "-o", "existing.c"),
                          ("input.ruv", "-o", "existing.c")]:
            with self.subTest(arguments=arguments):
                result = self.invoke(*arguments, environment=environment)
                self.assertEqual(result.returncode, 1, result.stderr)
                self.assertIn(b"generated C exceeds the 16 MiB limit", result.stderr)
                self.assertIn(b"input.ruv:1:", result.stderr)
                self.assertEqual(output.read_bytes(), b"preserved\n")
                self.assertFalse((self.directory / "arguments.json").exists())
                self.assert_no_temporary_outputs()

    def test_ordinary_nested_snapshots_still_execute(self):
        self.source.write_text(
            'object Pair { first: text; second: int; } launch() { '
            'keep item = Pair { second: 7, first: "value" }; '
            'walk outer in item { walk inner in item { show(outer.value, inner.value); } } }'
        )
        executable = self.compile()
        result = subprocess.run([str(executable)], capture_output=True, timeout=15)
        self.assertEqual((result.returncode, result.stdout, result.stderr),
                         (0, b"value value\nvalue 7\n7 value\n7 7\n", b""))


if __name__ == "__main__":
    unittest.main()
