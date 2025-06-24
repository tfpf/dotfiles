import platform
import sys

import IPython

c = get_config()  # noqa: F821
c.InteractiveShellApp.exec_PYTHONSTARTUP = True
c.InteractiveShellApp.ignore_cwd = False
c.InteractiveShellApp.pylab_import_all = False
c.BaseIPythonApplication.profile = "default"
c.TerminalIPythonApp.display_banner = True
c.InteractiveShell.autocall = 0
c.InteractiveShell.autoindent = True
c.InteractiveShell.automagic = True
c.InteractiveShell.banner1 = f"{platform.python_implementation()} {sys.version} on {sys.platform} with IPython {IPython.__version__}\n"
c.InteractiveShell.colors = "Linux"
c.TerminalInteractiveShell.auto_match = False
c.TerminalInteractiveShell.confirm_exit = False
c.TerminalInteractiveShell.display_completions = "readlinelike"
c.TerminalInteractiveShell.highlight_matching_brackets = True
c.TerminalInteractiveShell.mouse_support = False
c.Completer.backslash_combining_completions = True

if IPython.version_info < (9,):
    c.TerminalInteractiveShell.highlighting_style = "gruvbox-dark"
if IPython.version_info >= (9,):
    c.InteractiveShell.colors = "neutral"
    c.InteractiveShell.enable_tip = False
