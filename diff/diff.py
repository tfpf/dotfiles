#! /usr/bin/env python

from __future__ import annotations

import difflib
import functools
import itertools
import os
import subprocess
import sys
import tempfile
from collections import defaultdict
from pathlib import Path
from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from collections.abc import Iterable

html_begin = b"""
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN"
          "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">

<html>

<head>
    <meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
    <title>Diff</title>
    <style type="text/css">
        table.diff {font-family: monospace; border: medium;}
        .diff_header {background-color: #e0e0e0;}
        td.diff_header {text-align: right;}
        div {margin: 0px 4px 80px 4px;}
        details {display: inline-block;}
        summary {background-color: #e0e0e0; border-width: 1px 1px 0px 1px; border-style: solid; padding: 0px 4px 0px 4px; position: sticky; top: 0px;}
        .diff_next {background-color: #c0c0c0;}
        .diff_add {background-color: #aaffaa;}
        .diff_chg {background-color: #ffff77;}
        .diff_sub {background-color: #ffaaaa;}
    </style>
</head>

<body>
"""

html_end = b"</body></html>"

added_header = '<span style="color:green;">+++++</span>'
deleted_header = '<span style="color:red;">−−−−−</span>'  # noqa: RUF001
rename_detect_real_quick_threshold, rename_detect_quick_threshold, rename_detect_threshold = 0.5, 0.5, 0.5


def read_lines(self: Path) -> Iterable[str]:
    with open(self, encoding="utf-8") as self_reader:
        yield from self_reader


def read_words(self: Path) -> Iterable[str] | None:
    try:
        return self.read_text(encoding="utf-8").split()
    except UnicodeDecodeError:
        return None


Path.relative_to = functools.cache(Path.relative_to)


class Diff:
    """
    Compare two regular files directly or two directories recursively.
    """

    def __init__(self, left: str, right: str):
        self._left_directory, self._right_directory = Path(left).absolute(), Path(right).absolute()
        if self._left_directory.is_file() and self._right_directory.is_file():
            self._left_right_file_mapping = {self._left_directory: self._right_directory}
            self._left_files, self._right_files = set(), set()
            self._left_directory = Path(os.path.commonprefix([self._left_directory, self._right_directory]))
            self._right_directory = self._left_directory
        elif self._left_directory.is_dir() and self._right_directory.is_dir():
            self._matcher = difflib.SequenceMatcher(isjunk=None, autojunk=False)
            self._left_right_file_mapping = (
                self._changed_only_mapping | self._renamed_only_mapping | self._renamed_and_changed_mapping
            )
        else:
            raise ValueError("arguments must be two regular files or two directories")  # noqa: EM101, TRY003
        self._html_diff = difflib.HtmlDiff(wrapcolumn=119)

    @staticmethod
    def _files_in(directory: Path) -> dict[Path, Path]:
        """
        Recursively map the relative paths of all files in the given
        directory to their absolute paths.

        :param directory: Directory to traverse.
        :return: Files in the tree rooted at the given directory.
        """
        return {
            (file := Path(root) / file_name).relative_to(directory): file
            for root, _, file_names in os.walk(directory)
            for file_name in file_names
        }

    @property
    def _changed_only_mapping(self) -> dict[Path, Path]:
        """
        To each file in the left directory, trivially map the file in the right
        directory having the same relative path, if it exists.

        :return: Mapping between left and right directory files.
        """
        left_files, right_files = self._files_in(self._left_directory), self._files_in(self._right_directory)
        common_files = left_files.keys() & right_files.keys()
        left_right_file_mapping = {left_files[common_file]: right_files[common_file] for common_file in common_files}
        self._left_files = {v for k, v in left_files.items() if k not in common_files}
        self._right_files = {v for k, v in right_files.items() if k not in common_files}
        return left_right_file_mapping

    @functools.cached_property
    def _renamed_only_mapping(self) -> dict[Path, Path]:
        """
        To each file in the left directory, map the file in the right directory
        having the same contents, if it exists.

        :return: Mapping between left and right directory files.
        """
        left_directory_lookup = defaultdict(list)
        for left_file in self._left_files:
            left_file_contents = left_file.read_bytes()
            # Assume there are no collisions.
            left_directory_lookup[hash(left_file_contents)].append(left_file)

        left_right_file_mapping = {}
        for right_file in self._right_files.copy():
            right_file_contents = right_file.read_bytes()
            if not (identical_left_files := left_directory_lookup.get(hash(right_file_contents))):
                continue
            # Arbitrarily pick the last of the identical files.
            left_right_file_mapping[(identical_left_file := identical_left_files.pop())] = right_file
            self._left_files.remove(identical_left_file)
            self._right_files.remove(right_file)

        return left_right_file_mapping

    @property
    def _renamed_and_changed_mapping(self) -> dict[Path, Path]:
        """
        To each text file in the left directory, map the text file in the right
        directory having similar contents, if it exists.

        :return: Mapping between left and right directory files.
        """
        left_directory_matches = defaultdict(list)
        for left_file in self._left_files:
            if not (left_file_contents := read_words(left_file)):
                continue
            self._matcher.set_seq2(left_file_contents)
            for right_file in self._right_files:
                if not (right_file_contents := read_words(right_file)):
                    continue
                self._matcher.set_seq1(right_file_contents)
                if (
                    self._matcher.real_quick_ratio() > rename_detect_real_quick_threshold
                    and self._matcher.quick_ratio() > rename_detect_quick_threshold
                    and (similarity_ratio := self._matcher.ratio()) > rename_detect_threshold
                ):
                    left_directory_matches[left_file].append((similarity_ratio, right_file))
        for v in left_directory_matches.values():
            v.sort()

        # Ensure that the order in which we iterate over the files in the left
        # directory is such that the one having the greatest similarity ratio
        # with respect to any file in the right directory comes first.
        left_directory_matches = dict(
            sorted(left_directory_matches.items(), key=lambda kv: kv[1][-1][0], reverse=True)
        )

        left_right_file_mapping = {}
        for left_file, v in left_directory_matches.items():
            # Find the file in the right directory which is most similar to
            # this file in the left directory.
            for _, similar_right_file in reversed(v):
                if similar_right_file not in self._right_files:
                    continue
                left_right_file_mapping[left_file] = similar_right_file
                self._left_files.remove(left_file)
                self._right_files.remove(similar_right_file)
                break

        return left_right_file_mapping

    def report(self) -> Path:
        """
        Write HTML tables summarising the recursive differences between two
        directories.

        :return: File to which tables were written.
        """
        left_right_files = sorted(
            itertools.chain(
                ((left_file, None) for left_file in self._left_files),
                self._left_right_file_mapping.items(),
                ((None, right_file) for right_file in self._right_files),
            ),
            key=lambda lr: lr[1].relative_to(self._right_directory)
            if lr[1]
            else lr[0].relative_to(self._left_directory),
        )
        with tempfile.NamedTemporaryFile(delete=False, prefix="git-difftool-", suffix=".html") as writer:
            writer.write(html_begin)
            self._report(left_right_files, writer)
            writer.write(html_end)
        return Path(writer.name)

    def _report(self, left_right_files: Iterable[tuple[Path, None] | tuple[Path, Path] | tuple[None, Path]], writer):
        left_right_files_len = len(left_right_files)
        for pos, (left_file, right_file) in enumerate(left_right_files, 1):
            if left_file:
                from_desc, from_stat = str(left_file.relative_to(self._left_directory)), left_file.stat()
            else:
                from_desc = added_header
            if right_file:
                to_desc, to_stat = str(right_file.relative_to(self._right_directory)), right_file.stat()
            else:
                to_desc = deleted_header

            short_desc = []
            if left_file:
                short_desc.append(f"{from_stat.st_mode:o}")
            if from_desc != to_desc:
                short_desc.append(from_desc)
            if left_file and right_file and (from_desc != to_desc or from_stat.st_mode != to_stat.st_mode):
                short_desc.append("⟼")
            if not left_file or right_file and from_stat.st_mode != to_stat.st_mode:
                short_desc.append(f"{to_stat.st_mode:o}")
            short_desc.append(to_desc)
            writer.write(b"  <div><details open><summary><code>")
            writer.write((f"{pos}/{left_right_files_len} ■ " + " ".join(short_desc)).encode())
            if left_file in self._renamed_only_mapping or (
                left_file and right_file and left_file.read_bytes() == right_file.read_bytes()
            ):
                writer.write(" ■ identical</code></summary>\n  </details></div>\n".encode())
                continue
            if (not left_file and right_file and to_stat.st_size == 0) or (
                left_file and from_stat.st_size == 0 and not right_file
            ):
                writer.write(" ■ empty</code></summary>\n  </details></div>\n".encode())
                continue

            from_lines = read_lines(left_file) if left_file else []
            to_lines = read_lines(right_file) if right_file else []
            try:
                html_table = self._html_diff.make_table(
                    from_lines, to_lines, from_desc.center(64, "\u00a0"), to_desc.center(64, "\u00a0"), context=True
                )
                writer.write(b"</code></summary>\n")
                writer.write(html_table.encode())
            except UnicodeDecodeError:
                writer.write(" ■ binary</code></summary>\n".encode())
            writer.write(b"\n  </details></div>\n")


def main():
    diff = Diff(sys.argv[1], sys.argv[2])
    html_file = diff.report()
    print(html_file)  # noqa: T201
    subprocess.Popen(["/usr/bin/chromium", html_file.as_uri()])


if __name__ == "__main__":
    main()
