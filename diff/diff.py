#! /usr/bin/env python

import difflib
import collections
import itertools
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

    def report(self, directory_comparison = None):
        """
        Write an HTML table summarising the differences recorded between two
        directories. Then recurse on their subdirectories.
        :param directory_comparison: Directory comparison object.
        """
        common_files = self._left_directory_files.intersection(self._right_directory_files)
        self._left_directory_files -= common_files
        self._right_directory_files -= common_files
        left_directory_file_matches = collections.defaultdict(list)
        right_directory_file_matches = collections.defaultdict(list)
        for left_directory_file, right_directory_file in itertools.product(self._left_directory_files, self._right_directory_files):
            with open(os.path.join(self._left_directory, left_directory_file)) as left_directory_file_reader, open(os.path.join(self._right_directory, right_directory_file)) as right_directory_file_reader:
                left_directory_file_contents = left_directory_file_reader.read()
                right_directory_file_contents = right_directory_file_reader.read()
                self._matcher.set_seqs(left_directory_file_contents, right_directory_file_contents)
                if (similarity_ratio := self._matcher.ratio()) > 0.5:
                    left_directory_file_matches[left_directory_file].append((similarity_ratio, right_directory_file))
                    right_directory_file_matches[right_directory_file].append((similarity_ratio, left_directory_file))
        for v in left_directory_file_matches.values():
            v.sort()
        for v in right_directory_file_matches.values():
            v.sort()
        print(left_directory_file_matches)
        print(right_directory_file_matches)

        left_right_file_mapping = {}
        for left_directory_file, v in left_directory_file_matches.items():
            if left_directory_file in left_right_file_mapping or not v:
                continue
            for _, similar_right_directory_file in reversed(v):
                if similar_right_directory_file not in right_directory_file_matches:
                    continue
                _, similar_left_directory_file = right_directory_file_matches[similar_right_directory_file][-1]
                left_right_file_mapping[similar_left_directory_file] = similar_right_directory_file
                del right_directory_file_matches[similar_right_directory_file]
        for common_file in common_files:
            left_right_file_mapping[common_file] = common_file
        for item in left_right_file_mapping.items(): print(item)


def main():
    with tempfile.NamedTemporaryFile(delete=False, prefix="git-difftool-", suffix=".html") as writer:
        writer.write(html_begin)
        diff = Diff(sys.argv[1], sys.argv[2], writer)
        diff.report()
        writer.write(html_end)
    webbrowser.open("file://" + writer.name)


if __name__ == "__main__":
    main()
