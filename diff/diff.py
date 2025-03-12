#! /usr/bin/env python
import difflib
import filecmp
import os
import sys
import tempfile
import webbrowser


class Diff:
    """
    Compare two directories recursively. Don't attempt to detect renames.
    """

    def __init__(self, a: str, b: str, writer):
        self._directory_comparison = filecmp.dircmp(a, b, shallow=False)
        self._writer = writer
        self._html_diff = difflib.HtmlDiff()

    def _report(self, fromlines: list[str], tolines: list[str]):
        print(self._html_diff.make_table(fromlines, tolines).encode(), file=self._writer)

    def report(self, directory_comparison: filecmp.dircmp | None = None):
        """
        Write an HTML table summarising the differences recorded between two
        directories. Then recurse on their subdirectories.
        :param directory_comparison: Directory comparison object.
        """
        directory_comparison = directory_comparison or self._directory_comparison
        for deleted_file in directory_comparison.left_only:
            with open(os.path.join(directory_comparison.left, deleted_file)) as deleted_reader:
                self._report(deleted_reader.readlines(), [])
        for added_file in directory_comparison.right_only:
            with open(os.path.join(directory_comparison.right, added_file)) as added_reader:
                self._report([], added_reader.readlines())
        for changed_file in directory_comparison.diff_files:
            with (
                open(os.path.join(directory_comparison.left, changed_file)) as deleted_reader,
                open(os.path.join(directory_comparison.right, changed_file)) as added_reader,
            ):
                self._report(deleted_reader.readlines(), added_reader.readlines())
        for subdirectory_comparison in directory_comparison.subdirs.values():
            self.report(subdirectory_comparison)


def main():
    with tempfile.NamedTemporaryFile(delete=False, prefix="git-difftool-", suffix=".html") as writer:
        diff = Diff(sys.argv[1], sys.argv[2], writer)
        diff.report()
    webbrowser.open(writer.name)


if __name__ == "__main__":
    main()
