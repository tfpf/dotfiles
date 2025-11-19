import contextlib
import platform

# On Linux, the basic Python REPL reads my GNU Readline configuration file and
# uses coloured completion prefixes, so I prefer it to the PyREPL. On macOS and
# Windows, the configuration is not read. Hence, I prefer IPython, which
# provides similar completions (though without coloured prefixes).
if any(map(platform.platform(terse=True).startswith, ["macOS", "Windows"])):
    with contextlib.suppress(ImportError):
        import IPython

        if not IPython.get_ipython():
            IPython.start_ipython()
            raise SystemExit

import base64
import builtins
import calendar
import cmath
import collections
import copy
import csv
import ctypes
import datetime
import difflib
import dis
import enum
import errno
import filecmp
import fileinput
import fractions
import functools
import gc
import glob
import gzip
import hashlib
import http
import importlib
import io
import itertools
import json
import locale
import logging
import math
import multiprocessing
import numbers
import os
import pickle
import pprint
import random
import re
import secrets
import shutil
import signal
import ssl
import string
import subprocess
import sys
import sysconfig
import tarfile
import tempfile
import textwrap
import threading
import time
import timeit
import types
import typing
import uuid
import zipfile
import zoneinfo
from decimal import Decimal
from pathlib import Path, PosixPath, PurePath, PurePosixPath, PureWindowsPath

with contextlib.suppress(ImportError):
    import tkinter as tk
    from tkinter import ttk
