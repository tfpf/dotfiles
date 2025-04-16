#! /usr/bin/env python

import copy
import difflib
import fileinput
import functools
import itertools
import pathlib
import sys
import tempfile
import webbrowser
from collections import defaultdict
from collections.abc import Iterable
from multiprocessing import  Pool

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
        .added_header {color:green;}
        .deleted_header {color:red;}
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

added_header = '/<span class="added_header">+++++</span>'
deleted_header = '/<span class="deleted_header">−−−−−</span>'

rename_detect_real_quick_threshold, rename_detect_quick_threshold, rename_detect_threshold = 0.7, 0.6, 0.5


class Path(pathlib.Path):

    @functools.cache
    def read_bytes(self) -> bytes:
        return super().read_bytes()

    def read_lines(self) -> list[str]:
        return self.read_bytes().decode().splitlines()


class Diff:
    """
    Compare two directories recursively.
    """

    def __init__(self, left: str, right: str):
        self._left_directory = Path(left)
        self._right_directory = Path(right)
        self._html_diff = difflib.HtmlDiff(wrapcolumn=119)
        self._pool = Pool()
        self._left_right_file_mapping = (
            self._changed_not_renamed_mapping | self._renamed_not_changed_mapping | self._renamed_and_changed_mapping
        )

    @staticmethod
    def _files_in(directory: Path) -> dict[Path, Path]:
        """
        Recursively map the relative paths of all files in the given
        directory to their absolute paths.
        :param directory: Directory to traverse.
        :return: Files in the tree rooted at the given directory.
        """
        return {
            (file := root / file_name).relative_to(directory): file
            for root, _, file_names in directory.walk()
            for file_name in file_names
        }

    @property
    def _changed_not_renamed_mapping(self) -> dict[Path, Path]:
        """
        To each file in the left directory, trivially map the file in the right
        directory having the same relative path, if it exists.
        :return: Mapping between left and right directory files.
        """
        left_files, right_files = self._files_in(self._left_directory), self._files_in(self._right_directory)
        common_files = left_files.keys() & right_files.keys()
        left_right_file_mapping = {left_files[common_file]: right_files[common_file] for common_file in common_files}

        # Setting instance attributes outside the constructor is unusual, but
        # I'll allow it because this method is only ever called from the
        # constructor.
        self._left_files = {v for k, v in left_files.items() if k not in common_files}
        self._right_files = {v for k, v in right_files.items() if k not in common_files}

        return left_right_file_mapping

    @functools.cached_property
    def _renamed_not_changed_mapping(self) -> dict[Path, Path]:
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
            identical_left_file = identical_left_files.pop()
            left_right_file_mapping[identical_left_file] = right_file
            self._left_files.remove(identical_left_file)
            self._right_files.remove(right_file)

        return left_right_file_mapping

    @staticmethod
    def _renamed_and_changed_mapping_worker(left_file: Path, right_file: Path) -> int:
        try:
            left_file_contents, right_file_contents = left_file.read_text(), right_file.read_text()
        except UnicodeDecodeError:
            return 0
        matcher = difflib.SequenceMatcher(None, left_file_contents, right_file_contents, False)
        if (
            matcher.real_quick_ratio() > rename_detect_real_quick_threshold
            and matcher.quick_ratio() > rename_detect_quick_threshold
            and (similarity_ratio := matcher.ratio()) > rename_detect_threshold
        ):
            return similarity_ratio
        return 0

    @property
    def _renamed_and_changed_mapping(self) -> dict[Path, Path]:
        """
        To each text file in the left directory, map the text file in the right
        directory having similar contents, if it exists.
        :return: Mapping between left and right directory files.
        """
        left_directory_matches = defaultdict(list)
        iterable = itertools.product(self._left_files, self._right_files)
        results = self._pool.starmap_async(self._renamed_and_changed_mapping_worker, copy.copy(iterable)).get()
        for (left_file, right_file), similarity_ratio in zip(iterable, results, strict=True):
            if similarity_ratio > 0:
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

    def _report(self, left_right_files: Iterable[tuple[Path | None, Path | None]], writer):
        left_right_files_len = len(left_right_files)
        renamed_not_changed_mapping = self._renamed_not_changed_mapping
        for pos, (left_file, right_file) in enumerate(left_right_files, 1):
            from_desc = str(left_file.relative_to(self._left_directory)) if left_file else added_header
            to_desc = str(right_file.relative_to(self._right_directory)) if right_file else deleted_header
            writer.write(b'  <details open class="separator"><summary><code>')
            writer.write(f"{pos}/{left_right_files_len} ■ {from_desc} ■ {to_desc}".encode())
            if left_file in renamed_not_changed_mapping:
                writer.write(" ■ identical</code></summary>\n  </details>\n".encode())
                continue
            if (not left_file and right_file and right_file.stat().st_size == 0) or (
                left_file and left_file.stat().st_size == 0 and not right_file
            ):
                writer.write(" ■ empty</code></summary>\n  </details>\n".encode())
                continue

            try:
                from_lines = left_file.read_lines() if left_file else []
                to_lines = right_file.read_lines() if right_file else []
                html_table = self._html_diff.make_table(
                    from_lines, to_lines, from_desc.center(64, " "), to_desc.center(64, " "), context=True
                )
                writer.write(b"</code></summary>\n")
                writer.write(html_table.encode())
            except UnicodeDecodeError:
                writer.write(" ■ binary</code></summary>\n".encode())
            writer.write(b"\n  </details>\n")


def main():
    diff = Diff(sys.argv[1], sys.argv[2])
    html_file = diff.report()
    webbrowser.open(html_file.as_uri())
    print(Path.read_bytes.cache_info())


if __name__ == "__main__":
    main()
