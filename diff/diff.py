#! /usr/bin/env python

import filecmp
import os.path
import sys


def report(dc: filecmp.dircmp, path):
    print(path, dc.left_only, dc.right_only, dc.diff_files)
    for subdir, subdc in dc.subdirs.items():
        report(subdc, os.path.join(path, subdir))

def main():
    dc = filecmp.dircmp(sys.argv[1], sys.argv[2], shallow=False)
    report(dc, "")


if __name__ == "__main__":
    main()
