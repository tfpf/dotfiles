#! /usr/bin/env python

import filecmp
import os
import sys


def report(directory_comparison: filecmp.dircmp, path_prefix: str = ""):
    """
    Write an HTML snippet summarising the differences recorded between two
    directories. Then recurse on their subdirectories.
    :param directory_comparison: Directory comparison object.
    :param path_prefix: Common part of the path_prefix to the directories.
    """
    print(path_prefix, directory_comparison.left_only, directory_comparison.right_only,
          directory_comparison.diff_files)
    for subdirectory, subdirectory_comparison in directory_comparison.subdirs.items():
        report(subdirectory_comparison, os.path.join(path_prefix, subdirectory))


def main():
    directory_comparison = filecmp.dircmp(sys.argv[1], sys.argv[2], shallow=False)
    report(directory_comparison)


if __name__ == "__main__":
    main()
