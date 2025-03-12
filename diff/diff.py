#! /usr/bin/env python

import filecmp
import os
import sys


class Diff:
    """
    Compare two directories recursively.
    """

    def __init__(self, a: str, b: str):
        self._directory_comparison = filecmp.dircmp(a, b, shallow=True)

    def report(self, directory_comparison: filecmp.dircmp | None = None, path_prefix: str = ""):
        """
        Write an HTML snippet summarising the differences recorded between two
        directories. Then recurse on their subdirectories.
        :param directory_comparison: Directory comparison object.
        :param path_prefix: Common part of the path to the directories.
        """
        directory_comparison = directory_comparison or self._directory_comparison
        print(path_prefix, directory_comparison.left_only, directory_comparison.right_only,
              directory_comparison.diff_files)
        for subdirectory, subdirectory_comparison in directory_comparison.subdirs.items():
            self.report(subdirectory_comparison, os.path.join(path_prefix, subdirectory))


def main():
    diff = Diff(sys.argv[1], sys.argv[2])
    diff.report()


if __name__ == "__main__":
    main()
