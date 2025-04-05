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
        table.diff {font-family:monospace; border:medium;}
        .diff_header {background-color:#e0e0e0;}
        td.diff_header {text-align:right;}
        .diff_next {background-color:#c0c0c0;}
        .diff_add {background-color:#aaffaa;}
        .diff_chg {background-color:#ffff77;}
        .diff_sub {background-color:#ffaaaa;}
        .separator {margin-bottom:1cm;}
    </style>
</head>

<body>
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

    def __init__(self, left: str, right: str):
        self._left_directory = left
        self._left_directory_files = self._files_in(self._left_directory)
        self._right_directory = right
        self._right_directory_files = self._files_in(self._right_directory)
        self._common_files = self._left_directory_files & self._right_directory_files
        self._left_directory_files -= self._common_files
        self._right_directory_files -= self._common_files
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
    def _read_raw(source: str) -> bytes:
        """
        Read the given file raw.
        :param source: File name.
        :return: File contents.
        """
        with open(source, "rb") as source_reader:
            return source_reader.read()

    @staticmethod
    def _read_text(source: str) -> str:
        """
        Read the given file in text mode.
        :param source: File name.
        :return: File contents.
        """
        with open(source) as source_reader:
            return source_reader.read()

    @staticmethod
    def _read_lines(source: str) -> Iterable[str]:
        """
        Read the lines in the given file.
        :param source: File name.
        :return: File contents.
        """
        return fileinput.input(source)

    def _changed_not_renamed_mapping(self) -> dict[str, str]:
        """
        To each file in the left directory, trivially map the file in the right
        directory having the same path, if it exists.
        :return: Mapping between left and right directory files.
        """
        return {common_file: common_file for common_file in self._common_files}

    def _renamed_not_changed_mapping(self) -> dict[str, str]:
        """
        To each file in the left directory, map the file in the right directory
        having the same contents, if it exists.
        :return: Mapping between left and right directory files.
        """
        left_directory_lookup = collections.defaultdict(list)
        for left_directory_file in self._left_directory_files:
            left_directory_file_contents = self._read_raw(os.path.join(self._left_directory, left_directory_file))
            # Assume there are no collisions.
            left_directory_lookup[hash(left_directory_file_contents)].append(left_directory_file)

        left_right_file_mapping = {}
        for right_directory_file in self._right_directory_files.copy():
            right_directory_file_contents = self._read_raw(os.path.join(self._right_directory, right_directory_file))
            if not (identical_left_directory_files := left_directory_lookup.get(hash(right_directory_file_contents))):
                continue
            # Arbitrarily pick the last of the identical files.
            identical_left_directory_file = identical_left_directory_files.pop()
            left_right_file_mapping[identical_left_directory_file] = right_directory_file
            self._left_directory_files.remove(identical_left_directory_file)
            self._right_directory_files.remove(right_directory_file)

        return left_right_file_mapping

    def _renamed_and_changed_mapping(self) -> dict[str, str]:
        """
        To each file in the left directory, map the file in the right directory
        having similar contents, if it exists.
        :return: Mapping between left and right directory files.
        """
        left_directory_matches = collections.defaultdict(list)
        for left_directory_file in self._left_directory_files:
            left_directory_file_contents = self._read_text(os.path.join(self._left_directory, left_directory_file))
            # The second sequence undergoes preprocessing, which can be reused
            # when the first sequence changes. Hence, set the second sequence
            # here.
            self._matcher.set_seq2(left_directory_file_contents)
            for right_directory_file in self._right_directory_files:
                right_directory_file_contents = self._read_text(
                    os.path.join(self._right_directory, right_directory_file)
                )
                self._matcher.set_seq1(right_directory_file_contents)
                # Most code commits don't rename and change the same files.
                # Hence, a similarity ratio obtained cursorily will usually
                # suffice, thereby making the common case fast at the cost of
                # making the rare case slow.
                if (
                    self._matcher.real_quick_ratio() > rename_detect_threshold
                    and self._matcher.quick_ratio() > rename_detect_threshold
                    and (similarity_ratio := self._matcher.ratio()) > rename_detect_threshold
                ):
                    left_directory_matches[left_directory_file].append((similarity_ratio, right_directory_file))
        for v in left_directory_matches.values():
            v.sort()

        # Ensure that the order in which we iterate over the files in the left
        # directory is such that the one having the greatest similarity ratio
        # with respect to any file in the right directory comes first.
        left_directory_matches = dict(
            sorted(left_directory_matches.items(), key=lambda kv: kv[1][-1][0], reverse=True)
        )

        left_right_file_mapping = {}
        for left_directory_file, v in left_directory_matches.items():
            # Find the file in the right directory which is most similar to
            # this file in the left directory.
            for _, similar_right_directory_file in reversed(v):
                if similar_right_directory_file not in self._right_directory_files:
                    continue
                left_right_file_mapping[left_directory_file] = similar_right_directory_file
                self._left_directory_files.remove(left_directory_file)
                self._right_directory_files.remove(similar_right_directory_file)
                break

        return left_right_file_mapping

    def report(self) -> str:
        """
        Write HTML tables summarising the recursive differences between two
        directories.
        :return: File to which tables were written.
        """
        left_right_file_mapping = (
            self._changed_not_renamed_mapping()
            | self._renamed_not_changed_mapping()
            | self._renamed_and_changed_mapping()
        )
        left_right_directory_files = sorted(
            itertools.chain(
                ((file, "/deleted") for file in self._left_directory_files),
                left_right_file_mapping.items(),
                (("/added", file) for file in self._right_directory_files),
            ),
            key=lambda lr: lr[0] if not lr[0].startswith("/") else lr[1],
        )
        with tempfile.NamedTemporaryFile(delete=False, prefix="git-difftool-", suffix=".html") as writer:
            writer.write(html_begin)
            self._report(left_right_directory_files, writer)
            writer.write(html_end)
        return writer.name

    def _report(self, left_right_directory_files: Iterable[tuple[str, str]], writer):
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
            writer.write(b'<details open class="separator"><summary><code>')
            writer.write(f"{pos}/{left_right_directory_files_len} ■ {left_directory_file} ■ {right_directory_file}".encode())
            writer.write(b"</code></summary>\n")
            html_code = self._html_diff.make_table(from_lines, to_lines, left_directory_file, right_directory_file, context=True)
            writer.write(html_code.encode())
            writer.write(b"</details>\n")


def main():
    diff = Diff(sys.argv[1], sys.argv[2])
    html_file = diff.report()
    webbrowser.open("file://" + html_file)


if __name__ == "__main__":
    main()
