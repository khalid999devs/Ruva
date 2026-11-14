import unittest

from observation import ObservedCompilerTestCase


class CollectionTests(ObservedCompilerTestCase):
    def test_owned_collections_release_before_normal_and_early_exit(self):
        programs = [
            'object Item { name: text; count: int; } '
            'craft identity(item: Item) -> Item { keep copy = item; give copy; } '
            'launch() { vary items = ["one", "two"]; keep saved = pop(items); '
            'push(items, saved); items = [items[0], pop(items)]; '
            'vary item = Item { name: items[0], count: length(items) }; '
            'item = identity(item); item.name = item.name + "!"; '
            'walk field in item { item.name = "changed"; show(field.key, field.value); } }',
            'object Item { first: text; second: text; } '
            'craft select(item: Item) -> Item { vary values = [item.first, item.second]; '
            'walk value in values { walk field in item { '
            'keep copy = Item { first: value, second: field.value }; give copy; } } give item; } '
            'launch() { keep item = select(Item { first: "a", second: "b" }); '
            'show(item.first, item.second); }',
            'launch() { walk word in ["literal", "later"] { keep saved = word; give; } }',
        ]
        for source in programs:
            with self.subTest(source=source):
                result = self.run_observed(source)
                self.assertEqual(result.returncode, 0, result.stderr)

    def test_growth_replacement_and_snapshots_have_bounded_live_allocations(self):
        result = self.run_observed(
            'object Item { name: text; count: int; } '
            'craft identity(item: Item) -> Item { give item; } '
            'launch() { vary words: text[] = []; '
            'walk i from 0 until 2000 { push(words, "entry"); } '
            'walk i from 0 until 2000 { keep word = pop(words); } '
            'walk i from 0 until 2000 { words = ["one", "two"]; '
            'words = [words[0], pop(words)]; '
            'vary item = identity(Item { name: words[0], count: i }); '
            'walk field in item { item.name = "mutated"; keep copy = field.value; } } '
            'show(length(words)); }', peak_limit=2005
        )
        self.assertEqual((result.returncode, result.stdout, result.stderr), (0, b"2\n", b""))

    def test_each_allocation_failure_cleans_live_slots_records_and_snapshots(self):
        result = self.run_observed(
            'object Item { first: text; second: text; count: int; } '
            'craft identity(item: Item) -> Item { keep copy = item; give copy; } '
            'launch() { vary words = ["one", "two"]; '
            'walk i from 0 until 20 { push(words, "grown"); } '
            'keep item = identity(Item { first: words[0], second: "record", count: 7 }); '
            'words = [words[0] + "!", pop(words)]; '
            'walk field in item { keep pair = [field.key, field.value]; '
            'walk word in pair { keep copy = word + "!"; } } }'
        )
        self.assertEqual((result.returncode, result.stdout, result.stderr), (0, b"", b""))
        calls = int((self.directory / "allocation-count.txt").read_text())
        self.assertGreater(calls, 25)
        for failure in range(1, calls + 1):
            with self.subTest(allocation=failure):
                result = self.run_observed_executable(failure)
                self.assertEqual(result.returncode, 1, result.stderr)
                self.assertIn(b"out of memory", result.stderr)

    def test_fatal_cleanup_covers_rechecked_text_and_nested_snapshots(self):
        programs = [
            ('launch() { vary words = ["last"]; words[0] = pop(words); }', b"bounds"),
            ('object Item { first: text; second: text; } '
             'craft fail(item: Item, depth: int) -> text { '
             'walk field in item { keep values = [field.key, field.value]; '
             'check depth == 0 { give values[2]; } '
             'otherwise { give fail(item, depth - 1); } } give "empty"; } '
             'launch() { show(fail(Item { first: "a", second: "b" }, 5)); }', b"bounds"),
        ]
        for source, message in programs:
            with self.subTest(source=source):
                result = self.run_observed(source)
                self.assertEqual(result.returncode, 1, result.stderr)
                self.assertIn(message, result.stderr)

    def test_invalid_collection_operations_fail_before_cc(self):
        programs = [
            'launch() { keep items = [1]; keep copy = items; }',
            'launch() { vary items = [1]; items = items; }',
            'launch() { keep items = [1]; push(items, 2); }',
            'launch() { keep items = [1]; items[0] = 2; }',
            'launch() { vary items = [1]; push(items, 1.0); }',
            'launch() { vary items = [1]; walk item in items { show(pop(items)); } }',
            'launch() { vary items = [1]; walk item in items { '
            'walk other in [2] { push(items, 3); } } }',
            'launch() { vary items = [1]; walk item in items { items = []; } }',
            'object Item { name: text; } launch() { keep item = Item { name: "a" }; '
            'item.name = "b"; }',
            'object A { name: text; } object B { name: text; } '
            'craft use(item: A) -> void {} launch() { use(B { name: "b" }); }',
            'object Item { name: text; } launch() { walk field in Item { name: "a" } '
            '{ field.value = "b"; } }',
            'object Item { name: text; } launch() { walk field in Item { name: "a" } '
            '{ keep copy = field; } }',
        ]
        for source in programs:
            with self.subTest(source=source):
                self.source.write_text(source)
                result = self.invoke("input.ruv", environment=self.mock_compiler())
                self.assertNotEqual(result.returncode, 0)
                self.assertIn(b"error:", result.stderr)
                self.assertFalse((self.directory / "arguments.json").exists())
                self.assertFalse((self.directory / "a.out").exists())
                self.assert_no_temporary_outputs()


if __name__ == "__main__":
    unittest.main()
