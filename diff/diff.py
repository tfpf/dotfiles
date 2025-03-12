#! /usr/bin/env python

import filecmp
import os
import sys
import tempfile
import typing
import webbrowser


class Diff:
    """
    Compare two directories recursively. Don't attempt to detect renames.
    """

    def __init__(self, a: str, b: str, writer: typing.IO):
        self._directory_comparison = filecmp.dircmp(a, b, shallow=False)
        self._writer = writer

    def _report(self, from_file: str | None, to_file: str | None):
        print(from_file, to_file)

    def report(self, directory_comparison: filecmp.dircmp | None = None):
        """
        Write an HTML table summarising the differences recorded between two
        directories. Then recurse on their subdirectories.
        :param directory_comparison: Directory comparison object.
        """
        directory_comparison = directory_comparison or self._directory_comparison
        for deleted_file in directory_comparison.left_only:
            self._report(os.path.join(directory_comparison.left, deleted_file), None)
        for added_file in directory_comparison.right_only:
            self._report(None, os.path.join(directory_comparison.right, added_file))
        for changed_file in directory_comparison.diff_files:
            self._report(os.path.join(directory_comparison.left, changed_file),
                         os.path.join(directory_comparison.right, changed_file))
        for subdirectory_comparison in directory_comparison.subdirs.values():
            self.report(subdirectory_comparison)


def main():
    with tempfile.NamedTemporaryFile(delete=False, prefix="git-difftool-", suffix=".html") as writer:
        diff = Diff(sys.argv[1], sys.argv[2], writer)
        diff.report()
    webbrowser.open(writer.name)


if __name__ == "__main__":
    main()
