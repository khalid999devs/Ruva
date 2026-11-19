import os
import shutil
import struct
import subprocess
import unittest

from support import COMPILER, ROOT, WindowsCompilerTestCase


@unittest.skipUnless(os.name == "nt", "requires real Windows")
class WindowsDriverTests(WindowsCompilerTestCase):
    def test_compiler_is_x64_without_a_posix_runtime_dll(self):
        data = COMPILER.read_bytes()
        self.assertEqual(data[:2], b"MZ")
        offset = struct.unpack_from("<I", data, 60)[0]
        self.assertEqual(data[offset:offset + 4], b"PE\0\0")
        self.assertEqual(struct.unpack_from("<H", data, offset + 4)[0], 0x8664)
        objdump = shutil.which("objdump")
        self.assertIsNotNone(objdump, "Install the UCRT64 binutils package.")
        result = subprocess.run([objdump, "-p", str(COMPILER)], capture_output=True, timeout=30)
        self.assertEqual(result.returncode, 0, result.stderr)
        imports = result.stdout.lower()
        self.assertNotIn(b"msys-2.0.dll", imports)
        self.assertNotIn(b"cygwin1.dll", imports)

    def test_unicode_paths_backslashes_and_metacharacters(self):
        folder = self.directory / "code বাংলা & literal"
        folder.mkdir()
        source = folder / "source with spaces.ruv"
        self.source.rename(source)
        output = "code বাংলা & literal/output & literal.exe"
        executable = self.compile(str(source).replace("/", "\\"), output)
        result = self.execute(executable)
        self.assertEqual((result.returncode, result.stdout, result.stderr), (0, b"native\n", b""))

    def test_default_output_and_existing_output_replacement(self):
        result = self.invoke("input.ruv")
        self.assertEqual((result.returncode, result.stdout, result.stderr), (0, b"", b""))
        executable = self.directory / "a.exe"
        self.assertEqual(executable.read_bytes()[:2], b"MZ")
        self.source.write_bytes(b'launch() { show("replaced"); }\n')
        self.compile(output="a.exe")
        self.assertEqual(self.execute(executable).stdout, b"replaced\n")

    def test_native_process_arguments_and_failure_preserve_output(self):
        destination = self.directory / "keep & literal.exe"
        destination.write_bytes(b"previous output")
        for mode in ("failure", "empty", "invalid"):
            with self.subTest(mode=mode):
                result = self.invoke("input.ruv", "-o", destination,
                                     environment=self.mock_environment(mode))
                self.assertNotEqual(result.returncode, 0)
                self.assertEqual(destination.read_bytes(), b"previous output")
                arguments = (self.directory / "arguments.bin").read_bytes().split(b"\0")[:-1]
                decoded = [value.decode("utf-8") for value in arguments]
                self.assertIn("-std=c11", decoded)
                self.assertIn("-fno-fast-math", decoded)
                output = decoded[decoded.index("-o") + 1]
                self.assertTrue(output.endswith(".exe"), decoded)
                self.assertIn(".ruva-output-", output)
                self.assert_clean()
        environment = self.environment.copy()
        environment["CC"] = "gcc.exe & echo injected"
        result = self.invoke("input.ruv", environment=environment)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn(b"cannot start C compiler", result.stderr)
        self.assert_clean()

    def test_hardlinks_case_aliases_and_directory_outputs_are_protected(self):
        original = self.source.read_bytes()
        alias = self.directory / "alias.ruv"
        os.link(self.source, alias)
        for output in ("input.ruv", "INPUT.RUV", "alias.ruv"):
            result = self.invoke("--emit-c", "input.ruv", "-o", output)
            self.assertNotEqual(result.returncode, 0)
            self.assertIn(b"overwrite a source file", result.stderr)
            self.assertEqual(self.source.read_bytes(), original)
        (self.directory / "folder").mkdir()
        for output in ("folder", "missing/output.exe"):
            result = self.invoke("input.ruv", "-o", output)
            self.assertNotEqual(result.returncode, 0)
            self.assert_clean()

    def test_case_variants_of_one_module_share_record_identity(self):
        (self.directory / "model.ruv").write_bytes(
            b'share object Item { value: int; } '
            b'share craft make() -> Item { give Item { value: 12 }; }'
        )
        self.source.write_bytes(
            b'bring { Item } from "model.ruv"; bring { make } from "MODEL.RUV"; '
            b'launch() { keep item: Item = make(); show(item.value); }'
        )
        result = self.execute(self.compile())
        self.assertEqual((result.returncode, result.stdout, result.stderr), (0, b"12\n", b""))

    def test_relocated_compiler_uses_explicit_runtime_and_path_lookup(self):
        compiler = self.copy_compiler("relocated compiler.exe")
        environment = self.environment.copy()
        environment["RUVA_RUNTIME_ROOT"] = str(self.source.parents[0] / "missing")
        result = subprocess.run([str(compiler), str(self.source)], cwd=self.directory,
                                env=environment, capture_output=True, timeout=90)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn(b"cannot locate runtime", result.stderr)
        environment["RUVA_RUNTIME_ROOT"] = str(ROOT / "runtime")
        environment["PATH"] = str(self.directory) + os.pathsep + environment.get("PATH", "")
        result = subprocess.run([compiler.name, str(self.source), "-o", "relocated.exe"],
                                cwd=self.directory, env=environment, capture_output=True, timeout=90)
        self.assertEqual((result.returncode, result.stdout, result.stderr), (0, b"", b""))
        self.assertEqual(self.execute(self.directory / "relocated.exe").stdout, b"native\n")
        self.assert_clean()

    def test_invalid_workspace_and_unsupported_sanitizer_preserve_outputs(self):
        destination = self.directory / "previous.exe"
        destination.write_bytes(b"previous")
        environment = self.environment.copy()
        environment["TMPDIR"] = str(self.directory / "missing")
        result = self.invoke("input.ruv", "-o", destination, environment=environment)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn(b"cannot create compiler workspace", result.stderr)
        result = self.invoke("--sanitize", "input.ruv", "-o", destination)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn(b"--sanitize is unavailable", result.stderr)
        self.assertEqual(destination.read_bytes(), b"previous")
        self.assert_clean()
