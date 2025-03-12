#! /usr/bin/env python

import difflib
import filecmp
import fileinput
import os
import sys
import tempfile
import webbrowser


class Diff:
    """
    Compare two directories recursively. Does not handle renames and additions
    or deletions of directories correctly.
    """

    def __init__(self, a: str, b: str, writer):
        self._directory_comparison = filecmp.dircmp(a, b, shallow=False)
        self._writer = writer
        self._html_diff = difflib.HtmlDiff()

    def _read_lines(self, source: str) -> list[str]:
        """
        Read the lines in the given file.
        :param source: File name.
        :return: Lines in the file. Empty if it is a directory.
        """
        try:
            return [*fileinput.input(source)]
        except IsADirectoryError:
            return []

    def _report(self, from_lines: list[str], to_lines: list[str], from_desc: str, to_desc: str):
        if not from_lines and not to_lines:
            return
        self._writer.write(self._html_diff.make_table(from_lines, to_lines, from_desc, to_desc, context=True).encode())

    def report(self, directory_comparison: filecmp.dircmp | None = None):
        """
        Write an HTML table summarising the differences recorded between two
        directories. Then recurse on their subdirectories.
        :param directory_comparison: Directory comparison object.
        """
        directory_comparison = directory_comparison or self._directory_comparison
        for deleted_file in directory_comparison.left_only:
            deleted_file_path = os.path.join(directory_comparison.left, deleted_file)
            deleted_lines = self._read_lines(deleted_file_path)
            self._report(deleted_lines, [], deleted_file_path, "")
        for added_file in directory_comparison.right_only:
            added_file_path = os.path.join(directory_comparison.right, added_file)
            added_lines = self._read_lines(added_file_path)
            self._report([], added_lines, "", added_file_path)
        for changed_file in directory_comparison.diff_files:
            deleted_file_path = os.path.join(directory_comparison.left, changed_file)
            deleted_lines = self._read_lines(deleted_file_path)
            added_file_path = os.path.join(directory_comparison.right, changed_file)
            added_lines = self._read_lines(added_file_path)
            self._report(deleted_lines, added_lines, deleted_file_path, added_file_path)
        for subdirectory_comparison in directory_comparison.subdirs.values():
            self.report(subdirectory_comparison)


def main():
    with tempfile.NamedTemporaryFile(delete=False, prefix="git-difftool-", suffix=".html") as writer:
        diff = Diff(sys.argv[1], sys.argv[2], writer)
        diff.report()
    webbrowser.open(writer.name)


if __name__ == "__main__":
    main()
