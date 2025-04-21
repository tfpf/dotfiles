#! /usr/bin/env python

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

added_header = '<span style="color:green;">+++++</span>'
deleted_header = '<span style="color:red;">−−−−−</span>'  # noqa: RUF001
rename_detect_real_quick_threshold, rename_detect_quick_threshold, rename_detect_threshold = 0.5, 0.5, 0.5


class Path(pathlib.Path):
    relative_to = functools.cache(pathlib.Path.relative_to)

    def read_words(self) -> Iterable[str] | None:
        try:
            return super().read_text(encoding="utf-8").split()
        except UnicodeDecodeError:
            return None

    def read_lines(self) -> Iterable[str]:
        return fileinput.FileInput(self, encoding="utf-8")


class Diff:
    """
    Compare two directories recursively.
    """

    def __init__(self, left: str, right: str):
        self._left_directory = Path(left)
        self._right_directory = Path(right)
        self._html_diff = difflib.HtmlDiff(wrapcolumn=119)
        self._matcher = difflib.SequenceMatcher(isjunk=None, autojunk=False)
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

    @property
    def _renamed_and_changed_mapping(self) -> dict[Path, Path]:
        """
        To each text file in the left directory, map the text file in the right
        directory having similar contents, if it exists.
        :return: Mapping between left and right directory files.
        """
        left_directory_matches = defaultdict(list)
        for left_file in self._left_files:
            if not (left_file_contents := left_file.read_words()):
                continue
            self._matcher.set_seq2(left_file_contents)
            for right_file in self._right_files:
                if not (right_file_contents := right_file.read_words()):
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

    def _report(self, left_right_files: Iterable[tuple[Path | None, Path | None]], writer):
        left_right_files_len = len(left_right_files)
        for pos, (left_file, right_file) in enumerate(left_right_files, 1):
            if left_file:
                from_desc = str(left_file.relative_to(self._left_directory))
                from_mode = (from_stat := left_file.stat()).st_mode
            else:
                from_desc = added_header
            if right_file:
                to_desc = str(right_file.relative_to(self._right_directory))
                to_mode = (to_stat := right_file.stat()).st_mode
            else:
                to_desc = deleted_header

            if not left_file and right_file:
                short_desc = f"{from_desc} {to_mode:o} {to_desc}"
            elif left_file and not right_file:
                short_desc = f"{from_mode:o} {from_desc} {to_desc}"
            elif from_mode == to_mode:
                short_desc = (
                    f"{from_mode:o} {from_desc}" if from_desc == to_desc else f"{from_mode:o} {from_desc} ⟼ {to_desc}"
                )
            elif from_desc == to_desc:
                short_desc = f"{from_mode:o} ⟼ {to_mode:o} {from_desc}"
            else:
                short_desc = f"{from_mode:o} {from_desc} ⟼ {to_mode:o} {to_desc}"
            writer.write(b'  <details open style="margin-bottom:1cm;"><summary><code>')
            writer.write(f"{pos}/{left_right_files_len} ■ {short_desc}".encode())
            if left_file in self._renamed_not_changed_mapping or (
                left_file and right_file and left_file.read_bytes() == right_file.read_bytes()
            ):
                writer.write(" ■ identical</code></summary>\n  </details>\n".encode())
                continue
            if (not left_file and right_file and to_stat.st_size == 0) or (
                left_file and from_stat.st_size == 0 and not right_file
            ):
                writer.write(" ■ empty</code></summary>\n  </details>\n".encode())
                continue

            from_lines = left_file.read_lines() if left_file else []
            to_lines = right_file.read_lines() if right_file else []
            try:
                html_table = self._html_diff.make_table(
                    from_lines, to_lines, from_desc.center(64, "\u00a0"), to_desc.center(64, "\u00a0"), context=True
                )
                writer.write(b"</code></summary>\n")
                writer.write(html_table.encode())
            except UnicodeDecodeError:
                writer.write(" ■ binary</code></summary>\n".encode())
            writer.write(b"\n  </details>\n")


def main():
    diff = Diff(sys.argv[1], sys.argv[2])
    html_file = diff.report()
    print(html_file)  # noqa: T201
    webbrowser.open(html_file.as_uri())


if __name__ == "__main__":
    main()
