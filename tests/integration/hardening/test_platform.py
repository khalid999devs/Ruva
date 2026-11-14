import os
import subprocess
import unittest

from support import ROOT, CompilerTestCase


class PlatformTests(CompilerTestCase):
    def test_fast_math_is_rejected_by_runtime_platform_contract(self):
        source = self.directory / "platform.c"
        source.write_text('#include "compiler/common/platform.h"\nint main(void) { return 0; }\n')
        result = subprocess.run(
            [os.environ.get("CC", "cc"), "-std=c11", "-ffast-math", "-I", str(ROOT),
             "-c", str(source), "-o", str(self.directory / "platform.o")],
            capture_output=True, timeout=30,
        )
        self.assertNotEqual(result.returncode, 0)
        self.assertIn(b"Ruva requires floating-point builds without fast-math assumptions", result.stderr)
        self.assertFalse((self.directory / "platform.o").exists())

    def test_driver_is_independent_of_working_directory_and_locale(self):
        source = ('launch() { show(toReal("0.1"), toInt(" +08 "), '
                  'toText(0.1), toBool("true")); }')
        self.source.write_text(source)
        self.environment["LC_ALL"] = "C"
        executable = self.compile(self.source, "portable")
        result = subprocess.run([str(executable)], cwd=ROOT,
                                env=self.environment, capture_output=True, timeout=15)
        self.assertEqual((result.returncode, result.stdout, result.stderr),
                         (0, b"0.10000000000000001 8 0.10000000000000001 true\n", b""))


if __name__ == "__main__":
    unittest.main()
