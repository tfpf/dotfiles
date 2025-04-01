#! /usr/bin/env python

import collections
import difflib
import fileinput
import itertools
import os
import sys
import tempfile
import webbrowser
from collections.abc import Iterable

rename_detect_threshold = 0.5

html_begin = b"""
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN"
          "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">

<html>

<head>
    <meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
    <title>Diff</title>
    <style type="text/css">
        table.diff {font-family:Monospace; border:medium;}
        .diff_header {background-color:#e0e0e0}
        td.diff_header {text-align:right}
        .diff_next {background-color:#c0c0c0}
        .diff_add {background-color:#aaffaa}
        .diff_chg {background-color:#ffff77}
        .diff_sub {background-color:#ffaaaa}
        .separator {margin:2cm}
    </style>
</head>

<body>
"""

html_separator = b"""
    <p class="separator"></p>
"""

html_end = b"""
    <table class="diff" summary="Legends">
        <tr><th colspan="2">Legends</th></tr>
        <tr><td><table border="" summary="Colours">
            <tr><th>Colours</th></tr>
            <tr><td class="diff_add">&nbsp;Added&nbsp;</td></tr>
            <tr><td class="diff_chg">Changed</td></tr>
            <tr><td class="diff_sub">Deleted</td></tr>
        </table></td>
        <td><table border="" summary="Links">
            <tr><th colspan="2">Links</th></tr>
            <tr><td>(f)irst change</td></tr>
            <tr><td>(n)ext change</td></tr>
            <tr><td>(t)op</td></tr>
        </table></td></tr>
    </table>
</body>

</html>
"""


class Diff:
    """
    Compare two directories recursively.
    """

    def __init__(self, left: str, right: str, writer):
        self._left_directory = left
        self._left_directory_files = self._files_in(self._left_directory)
        self._right_directory = right
        self._right_directory_files = self._files_in(self._right_directory)
        self._common_files = self._left_directory_files.intersection(self._right_directory_files)
        self._left_directory_files -= self._common_files
        self._right_directory_files -= self._common_files
        self._writer = writer
        self._matcher = difflib.SequenceMatcher(autojunk=False)
        self._html_diff = difflib.HtmlDiff(wrapcolumn=119)

    @staticmethod
    def _files_in(directory: str) -> set[str]:
        """
        Recursively list the relative paths of all files in the given
        directory.
        :param directory: Absolute directory path.
        :return: Files in the tree rooted at the given directory.
        """
        return {
            os.path.join(root, file_name).removeprefix(directory)
            for root, _, file_names in os.walk(directory)
            for file_name in file_names
        }

    @staticmethod
    def _read_lines(source: str) -> Iterable[str]:
        """
        Read the lines in the given file.
        :param source: File name.
        :return: Lines in the file.
        """
        return fileinput.input(source)

    def _report(self, from_lines: Iterable[str], to_lines: Iterable[str], from_desc: str, to_desc: str):
        html_code = self._html_diff.make_table(from_lines, to_lines, from_desc, to_desc, context=True)
        self._writer.write(html_code.encode())

    def report(self):
        """
        Write HTML tables summarising the recursive differences between two
        directories.
        """
        left_directory_file_matches = collections.defaultdict(list)
        for left_directory_file, right_directory_file in itertools.product(
            self._left_directory_files, self._right_directory_files
        ):
            with (
                open(os.path.join(self._left_directory, left_directory_file)) as left_directory_file_reader,
                open(os.path.join(self._right_directory, right_directory_file)) as right_directory_file_reader,
            ):
                left_directory_file_contents = left_directory_file_reader.read()
                right_directory_file_contents = right_directory_file_reader.read()
                self._matcher.set_seqs(left_directory_file_contents, right_directory_file_contents)
                if (similarity_ratio := self._matcher.ratio()) > rename_detect_threshold:
                    left_directory_file_matches[left_directory_file].append((similarity_ratio, right_directory_file))
        for v in left_directory_file_matches.values():
            v.sort()

        # Ensure that the order in which we iterate over the files in the left
        # directory is such that the one having the greatest similarity ratio
        # with respect to any file in the right directory comes first.
        left_directory_file_matches = dict(
            sorted(left_directory_file_matches.items(), key=lambda kv: kv[1][-1][0] if kv[1] else 0)
        )

        # Files which were changed without renaming.
        left_right_file_mapping = {common_file: common_file for common_file in self._common_files}

        # Detect renames by mapping a file in the left directory to one in the
        # right.
        for left_directory_file, v in left_directory_file_matches.items():
            if not v or left_directory_file not in self._left_directory_files:
                continue

            # Find the file in the right directory which is most similar to
            # this file in the left directory.
            for _, similar_right_directory_file in reversed(v):
                if similar_right_directory_file not in self._right_directory_files:
                    continue
                left_right_file_mapping[left_directory_file] = similar_right_directory_file
                self._left_directory_files.remove(left_directory_file)
                self._right_directory_files.remove(similar_right_directory_file)
                break

        left_right_directory_files = sorted(
            itertools.chain(
                ((file, "/deleted") for file in self._left_directory_files),
                left_right_file_mapping.items(),
                (("/added", file) for file in self._right_directory_files),
            ),
            key=lambda lr: lr[0] if not lr[0].startswith("/") else lr[1],
        )
        left_right_directory_files_len = len(left_right_directory_files)
        for pos, (left_directory_file, right_directory_file) in enumerate(left_right_directory_files, 1):
            if left_directory_file == "/added":
                from_lines = []
            else:
                from_lines = self._read_lines(os.path.join(self._left_directory, left_directory_file))
            if right_directory_file == "/deleted":
                to_lines = []
            else:
                to_lines = self._read_lines(os.path.join(self._right_directory, right_directory_file))
            self._writer.write(b"<details open><summary><code>")
            self._writer.write(
                f"{pos}/{left_right_directory_files_len} ■ {left_directory_file} ■ {right_directory_file}".encode()
            )
            self._writer.write(b"</code></summary>\n")
            self._report(from_lines, to_lines, left_directory_file, right_directory_file)
            self._writer.write(b"</details>\n")
            self._writer.write(html_separator)


def main():
    with tempfile.NamedTemporaryFile(delete=False, prefix="git-difftool-", suffix=".html") as writer:
        writer.write(html_begin)
        diff = Diff(sys.argv[1], sys.argv[2], writer)
        diff.report()
        writer.write(html_end)
    webbrowser.open("file://" + writer.name)


if __name__ == "__main__":
    main()
