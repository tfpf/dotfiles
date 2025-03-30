#! /usr/bin/env python

import difflib
import fileinput
import os
import sys
import tempfile
import webbrowser

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
    Compare two directories recursively. Does not handle renames and additions
    or deletions of directories correctly.
    """

    def __init__(self, a: str, b: str, writer):
        self._directory_comparison = filecmp.dircmp(a, b, shallow=False)
        self._writer = writer
        self._html_diff = difflib.HtmlDiff(wrapcolumn=119)

    @staticmethod
    def _read_lines(source: str) -> list[str]:
        """
        Read the lines in the given file.
        :param source: File name.
        :return: Lines in the file. Empty if it is a directory.
        """
        try:
            return [*fileinput.input(source)]
        except IsADirectoryError:
            return []

    @staticmethod
    def _remove_temporary_directory_prefix(file_path: str):
        file_path_len = len(file_path)
        try:
            index_of_after_left = file_path.index("/left/") + 6
        except ValueError:
            index_of_after_left = file_path_len
        try:
            index_of_after_right = file_path.index("/right/") + 7
        except ValueError:
            index_of_after_right = file_path_len
        index_of_after_left_or_right = min(index_of_after_left, index_of_after_right)
        if index_of_after_left_or_right != file_path_len:
            return file_path[index_of_after_left_or_right:]
        return file_path

    def _report(self, from_lines: list[str], to_lines: list[str], from_desc: str, to_desc: str):
        if not from_lines and not to_lines:
            return
        from_desc = self._remove_temporary_directory_prefix(from_desc)
        to_desc = self._remove_temporary_directory_prefix(to_desc)
        html_code = self._html_diff.make_table(from_lines, to_lines, from_desc, to_desc, context=True)
        self._writer.write(f"<details open><summary><code>{from_desc} | {to_desc}</code></summary>\n".encode())
        self._writer.write(html_code.encode())
        self._writer.write(b"</details>\n")
        self._writer.write(html_separator)

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
            self._report(deleted_lines, [], deleted_file_path, "[file deleted]")
        for added_file in directory_comparison.right_only:
            added_file_path = os.path.join(directory_comparison.right, added_file)
            added_lines = self._read_lines(added_file_path)
            self._report([], added_lines, "[file added]", added_file_path)
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
        writer.write(html_begin)
        diff = Diff(sys.argv[1], sys.argv[2], writer)
        diff.report()
        writer.write(html_end)
    webbrowser.open("file://" + writer.name)


if __name__ == "__main__":
    main()
