#! /usr/bin/env python

import collections
import difflib
import fileinput
import itertools
import os
import sys
import tempfile
import webbrowser

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
        self._writer = writer
        self._matcher = difflib.SequenceMatcher(autojunk=False)
        self._html_diff = difflib.HtmlDiff(wrapcolumn=119)

    def _files_in(self, directory: str) -> set[str]:
        """
        Recursively list the relative paths of all files in the given
        directory.
        :param directory: Directory path.
        :return: Files in the tree rooted at the given directory.
        """
        files = set()
        for root, _, file_names in os.walk(directory):
            for file_name in file_names:
                file = os.path.join(root, file_name).removeprefix(directory)
                files.add(file)
        return files

    @staticmethod
    def _read_lines(source: str) -> list[str]:
        """
        Read the lines in the given file.
        :param source: File name.
        :return: Lines in the file.
        """
        return [*fileinput.input(source)]

    def _report(self, from_lines: list[str], to_lines: list[str], from_desc: str, to_desc: str):
        if not from_lines and not to_lines:
            return
        html_code = self._html_diff.make_table(from_lines, to_lines, from_desc, to_desc, context=True)
        self._writer.write(f"<details open><summary><code>{from_desc} | {to_desc}</code></summary>\n".encode())
        self._writer.write(html_code.encode())
        self._writer.write(b"</details>\n")
        self._writer.write(html_separator)

    def report(self):
        """
        Write an HTML table summarising the recursive differences between two
        directories.
        """
        common_files = self._left_directory_files.intersection(self._right_directory_files)
        self._left_directory_files -= common_files
        self._right_directory_files -= common_files
        left_directory_file_matches = collections.defaultdict(list)
        right_directory_file_matches = collections.defaultdict(list)
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
                    right_directory_file_matches[right_directory_file].append((similarity_ratio, left_directory_file))
        for v in left_directory_file_matches.values():
            v.sort()
        left_directory_file_matches = dict(
            sorted(left_directory_file_matches.items(), key=lambda kv: kv[1][-1][0] if kv[1] else 0)
        )
        for v in right_directory_file_matches.values():
            v.sort()

        left_right_file_mapping = {}
        for left_directory_file, v in left_directory_file_matches.items():
            if not v or left_directory_file in left_right_file_mapping:
                continue
            for _, similar_right_directory_file in reversed(v):
                if similar_right_directory_file not in right_directory_file_matches:
                    continue
                _, similar_left_directory_file = right_directory_file_matches[similar_right_directory_file][-1]
                left_right_file_mapping[similar_left_directory_file] = similar_right_directory_file
                del right_directory_file_matches[similar_right_directory_file]
                self._left_directory_files.remove(similar_left_directory_file)
                self._right_directory_files.remove(similar_right_directory_file)
        for common_file in common_files:
            left_right_file_mapping[common_file] = common_file

        for file in sorted([*self._left_directory_files, *self._right_directory_files, *left_right_file_mapping]):
            if file in self._left_directory_files:
                self._report(self._read_lines(os.path.join(self._left_directory, file)), [], file, "[deleted]")
            elif file in self._right_directory_files:
                self._report([], self._read_lines(os.path.join(self._right_directory, file)), "[added]", file)
            else:
                self._report(
                    self._read_lines(os.path.join(self._left_directory, file)),
                    self._read_lines(os.path.join(self._right_directory, left_right_file_mapping[file])),
                    file,
                    file,
                )


def main():
    with tempfile.NamedTemporaryFile(delete=False, prefix="git-difftool-", suffix=".html") as writer:
        writer.write(html_begin)
        diff = Diff(sys.argv[1], sys.argv[2], writer)
        diff.report()
        writer.write(html_end)
    webbrowser.open("file://" + writer.name)


if __name__ == "__main__":
    main()
