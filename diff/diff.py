#! /usr/bin/env python

import filecmp
import os
import sys


class Diff:
    """
    Compare two directories recursively. Don't attempt to detect renames.
    """

    def __init__(self, a: str, b: str):
        self._directory_comparison = filecmp.dircmp(a, b, shallow=True)

    def report(self, directory_comparison: filecmp.dircmp | None = None, common_path: str = ""):
        """
        Write an HTML table summarising the differences recorded between two
        directories. Then recurse on their subdirectories.
        :param directory_comparison: Directory comparison object.
        :param common_path: Common part of the path to the directories.
        """
        directory_comparison = directory_comparison or self._directory_comparison
        print(common_path, directory_comparison.left_only, directory_comparison.right_only,
              directory_comparison.diff_files)
        for subdirectory, subdirectory_comparison in directory_comparison.subdirs.items():
            self.report(subdirectory_comparison, os.path.join(common_path, subdirectory))


def main():
    diff = Diff(sys.argv[1], sys.argv[2])
    diff.report()


if __name__ == "__main__":
    main()
