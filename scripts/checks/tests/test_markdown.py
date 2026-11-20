import unittest

import support  # noqa: F401

from nexum_checks import markdown


class MarkdownTests(unittest.TestCase):
    def test_front_matter_is_read_without_comments(self) -> None:
        text = "---\nid: 0001\nstatus: aceita   # a comment\nempty:\n---\nkey: outside\n"
        self.assertEqual(markdown.read_front_matter(text), {"id": "0001", "status": "aceita", "empty": ""})

    def test_a_file_without_front_matter_has_no_values(self) -> None:
        self.assertEqual(markdown.read_front_matter("# Title\nkey: value\n"), {})

    def test_links_report_their_lines(self) -> None:
        self.assertEqual(list(markdown.links("first\n[a](b.md)\n![c](d.png)\n")), [("b.md", 2), ("d.png", 3)])

    def test_slug_follows_github(self) -> None:
        self.assertEqual(markdown.slug("Conteúdo de terceiros"), "conteúdo-de-terceiros")
        self.assertEqual(markdown.slug("A `code` & [link](x.md)!"), "a-code--link")

    def test_repeated_headings_get_a_number(self) -> None:
        self.assertEqual(markdown.anchors("# A\n## A\n### A\n"), {"a", "a-1", "a-2"})

    def test_headings_inside_code_are_ignored(self) -> None:
        self.assertEqual(markdown.anchors("```\n# Not a heading\n```\n# Real\n"), {"real"})


if __name__ == "__main__":
    unittest.main()
