import os
import subprocess
from textwrap import dedent
import unittest

from support import ROOT, CompilerTestCase


class ModuleTests(CompilerTestCase):
    def write_files(self, files):
        for name, source in files.items():
            path = self.directory / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(dedent(source).lstrip(), encoding="utf-8")

    def execute(self, executable):
        return subprocess.run([str(executable)], cwd=self.directory,
                              env=self.environment, capture_output=True, timeout=15)

    def assert_rejected(self, source, message):
        output = self.directory / "previous.exe"
        output.write_bytes(b"previous output")
        environment = self.environment.copy()
        environment["CC"] = str(self.directory / "unavailable-compiler.exe")
        for arguments in (("--check", source), (source, "-o", output)):
            result = self.invoke(*arguments, environment=environment)
            self.assertEqual(result.returncode, 1, result.stderr)
            self.assertIn(message, result.stderr)
            self.assertNotIn(b"cannot start C compiler", result.stderr)
            self.assertEqual(output.read_bytes(), b"previous output")
            self.assert_no_temporary_outputs()
        return result

    def test_public_example_from_an_unrelated_working_directory(self):
        executable = self.compile(ROOT / "examples/modules/main.ruv", "modules.exe")
        result = self.execute(executable)
        self.assertEqual((result.returncode, result.stdout, result.stderr),
                         (0, b"Asha: 42 (pass)\n", b""))

    def test_diamond_preserves_records_constants_and_private_helpers(self):
        self.write_files({
            "project বাংলা with spaces/main.ruv": '''
                bring { make } from "services/create.ruv";
                bring { describe } from "services/render.ruv";
                bring { Item } from "model/./item.ruv";
                bring { label, count, ratio, active } from "settings.ruv";

                launch() {
                    keep first: Item = make("Ada", count);
                    show(label, ratio, active, describe(first));
                    vary total = 0;
                    walk i from 0 until 256 {
                        keep item: Item = make("copy", i);
                        keep description = describe(item);
                        check description == "shared:copy!:" + toText(i) {
                            total = total + item.value;
                        }
                    }
                    show(first.name, first.value, total);
                }
            ''',
            "project বাংলা with spaces/model/item.ruv": '''
                share object Item { name: text; value: int; }
            ''',
            "project বাংলা with spaces/settings.ruv": '''
                share keep label = "shared";
                share keep count = 10;
                share keep ratio = 1.5;
                share keep active = true;
            ''',
            "project বাংলা with spaces/services/create.ruv": '''
                bring { Item } from "../model/item.ruv";
                bring { active } from "../settings.ruv";

                share craft make(name: text, value: int) -> Item {
                    give Item { name: name + helper(), value: value };
                }

                craft helper() -> text {
                    check active { give "!"; } otherwise { give "?"; }
                }
            ''',
            "project বাংলা with spaces/services/render.ruv": '''
                bring { Item } from "../model/./item.ruv";
                bring { label } from ".././settings.ruv";

                craft helper() -> text { give label + ":"; }

                share craft describe(item: Item) -> text {
                    give helper() + item.name + ":" + toText(item.value);
                }
            ''',
        })
        source = "project বাংলা with spaces/main.ruv"
        checked = self.invoke("--check", source)
        self.assertEqual((checked.returncode, checked.stdout, checked.stderr), (0, b"", b""))
        result = self.execute(self.compile(source, "diamond.exe"))
        self.assertEqual((result.returncode, result.stdout, result.stderr),
                         (0, b"shared 1.5 true shared:Ada!:10\nAda! 10 32640\n", b""))

    def test_shared_functions_use_forward_recursion_and_private_dependencies(self):
        self.write_files({
            "main.ruv": '''
                bring { even, greeting, announce } from "library/api.ruv";
                launch() {
                    announce(greeting("Ruva"));
                    show(even(0), even(9), even(10));
                }
            ''',
            "library/api.ruv": '''
                bring { suffix } from "details/text.ruv";

                share craft even(value: int) -> bool {
                    check value == 0 { give true; }
                    give odd(value - 1);
                }

                craft odd(value: int) -> bool {
                    check value == 0 { give false; }
                    give even(value - 1);
                }

                share craft greeting(value: text) -> text { give prefix + value + suffix(); }
                share craft announce(value: text) -> void { show(value); }
                keep prefix = "Hello ";
            ''',
            "library/details/text.ruv": '''
                share craft suffix() -> text { give "!"; }
            ''',
        })
        result = self.execute(self.compile("main.ruv", "recursion.exe"))
        self.assertEqual((result.returncode, result.stdout, result.stderr),
                         (0, b"Hello Ruva!\ntrue false true\n", b""))

    def test_identically_shaped_records_from_different_modules_remain_distinct(self):
        self.write_files({
            "main.ruv": '''
                bring { accept } from "left.ruv";
                bring { make } from "right.ruv";
                launch() { show(accept(make())); }
            ''',
            "left.ruv": '''
                object Item { value: int; }
                share craft accept(item: Item) -> int { give item.value; }
            ''',
            "right.ruv": '''
                object Item { value: int; }
                share craft make() -> Item { give Item { value: 7 }; }
            ''',
        })
        self.assert_rejected("main.ruv", b"argument type")

    def test_import_visibility_and_collisions_fail_before_native_compilation(self):
        cases = [
            ('bring { hidden } from "lib.ruv";', "keep hidden = 1;", b"private"),
            ('bring { hidden } from "lib.ruv";',
             "craft hidden() -> int { give 1; }", b"private"),
            ('bring { Hidden } from "lib.ruv";', "object Hidden { value: int; }", b"private"),
            ('bring { absent } from "lib.ruv";', "share keep value = 1;", b"not declared"),
            ('bring { value, value } from "lib.ruv";', "share keep value = 1;", b"duplicate"),
            ('bring { value } from "lib.ruv"; bring { value } from "./lib.ruv";',
             "share keep value = 1;", b"duplicate"),
            ('bring { value } from "lib.ruv"; keep value = 2;',
             "share keep value = 1;", b"duplicate"),
            ('bring { value } from "lib.ruv"; bring { value } from "other.ruv";',
             "share keep value = 1;", b"duplicate"),
            ('bring { value } from "lib.ruv";',
             'bring { value } from "other.ruv";', b"not declared"),
            ('bring { show } from "lib.ruv";',
             "share craft show() -> void {}", b"built-in names"),
        ]
        for index, (imports, library, message) in enumerate(cases):
            with self.subTest(case=index):
                self.write_files({
                    "main.ruv": imports + " launch() {}",
                    "lib.ruv": library,
                    "other.ruv": "share keep value = 2;",
                })
                self.assert_rejected("main.ruv", message)

    def test_imported_errors_retain_the_dependency_file_and_line(self):
        cases = [
            ('share keep value = "ok";\n@\n', b"unknown source character"),
            ('share keep value = "ok";\ncraft broken(value int) -> int { give 1; }\n', b"expected"),
            ('share keep value = "ok";\ncraft broken() -> int { give missing; }\n', b"unknown"),
        ]
        for source, message in cases:
            with self.subTest(source=source):
                self.write_files({
                    "main.ruv": 'bring { value } from "parts/broken.ruv"; launch() {}',
                    "parts/broken.ruv": source,
                })
                result = self.assert_rejected("main.ruv", message)
                self.assertIn(b"broken.ruv:2:", result.stderr)

    def test_missing_nested_dependency_and_cycles_are_positioned_errors(self):
        cases = [
            ('bring { value } from "missing.ruv"; share keep first = 1;', b"cannot resolve"),
            ('bring { value } from "../../main.ruv"; share keep first = 1;', b"circular"),
            ('bring { value } from "../second.ruv"; share keep first = 1;', b"circular"),
        ]
        for library, message in cases:
            with self.subTest(message=message, library=library):
                self.write_files({
                    "main.ruv": 'bring { first } from "parts/nested/first.ruv"; launch() {}',
                    "parts/nested/first.ruv": library,
                    "parts/second.ruv": '''
                        bring { first } from "nested/./first.ruv";
                        share keep value = 1;
                    ''',
                })
                result = self.assert_rejected("main.ruv", message)
                self.assertIn(b".ruv:1:", result.stderr)

    def test_imported_modules_reject_entry_points_and_mutable_globals(self):
        for library, message in [
            ("launch() {}", b"launch"),
            ("vary value = 1;", b"top-level"),
            ("share keep value = 1 + 2;", b"after constant"),
        ]:
            with self.subTest(library=library):
                self.write_files({
                    "main.ruv": 'bring { value } from "lib.ruv"; launch() {}',
                    "lib.ruv": library,
                })
                self.assert_rejected("main.ruv", message)

    def test_native_and_c_outputs_cannot_overwrite_a_dependency(self):
        self.write_files({
            "main.ruv": 'bring { value } from "lib.ruv"; launch() { show(value); }',
            "lib.ruv": "share keep value = 7;",
        })
        library = self.directory / "lib.ruv"
        original = library.read_bytes()
        alias = self.directory / "alias.ruv"
        os.link(library, alias)
        for output in (library, alias):
            for mode in ((), ("--emit-c",)):
                with self.subTest(output=output, mode=mode):
                    result = self.invoke(*mode, "main.ruv", "-o", output)
                    self.assertEqual(result.returncode, 1, result.stderr)
                    self.assertIn(b"overwrite a source file", result.stderr)
                    self.assertEqual(library.read_bytes(), original)
                    self.assertEqual(alias.read_bytes(), original)
                    self.assert_no_temporary_outputs()

    def test_dependency_edits_are_seen_on_each_compilation(self):
        self.write_files({
            "main.ruv": 'bring { value } from "lib.ruv"; launch() { show(value); }',
            "lib.ruv": "share keep value = 1;",
        })
        executable = self.compile("main.ruv", "updated.exe")
        self.assertEqual(self.execute(executable).stdout, b"1\n")
        self.write_files({"lib.ruv": "keep value = 2;"})
        self.assert_rejected("main.ruv", b"private")
        self.assertEqual(self.execute(executable).stdout, b"1\n")
        self.write_files({"lib.ruv": "share keep value = 3;"})
        result = self.execute(self.compile("main.ruv", "updated.exe"))
        self.assertEqual((result.returncode, result.stdout, result.stderr), (0, b"3\n", b""))


if __name__ == "__main__":
    unittest.main()
