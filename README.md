[![style](https://github.com/tfpf/dotfiles/actions/workflows/style.yml/badge.svg)](https://github.com/tfpf/dotfiles/actions/workflows/style.yml)
[![package](https://github.com/tfpf/dotfiles/actions/workflows/package.yml/badge.svg)](https://github.com/tfpf/dotfiles/actions/workflows/package.yml)

<p align="center">
  <img src="res/certified_human.svg" />
</p>

No part of the code in this repository has been written by or in consultation with artificial intelligence chatbots such as (but not limited to) Bard and ChatGPT.

---

These are the configuration files I use on Linux, macOS and Windows to set up a consistent development environment.

[`custom-prompt`](custom-prompt) contains code to create a prompt for Bash or Zsh with information about the current
Git repository (from `__git_ps1`) and the current Python virtual environment, and report the running time of long
commands. A session may typically look like

```console
┌[tfpf  Alpine ~/Documents/projects/dotfiles]  main
└─% pipenv shell

┌[tfpf  Alpine ~/Documents/projects/dotfiles]  main %  dotfiles
└─▶% exit
                                         pipenv shell  00:43.735

┌[tfpf  Alpine ~/Documents/projects/dotfiles]  main %
└─%
```

but with colours. (The number of right-pointing triangles just before the prompt symbol is one less than the current
shell level.) If the terminal is not the active window when a long command terminates, a desktop notification will also
be sent; this is done using OSC 777 on macOS and Windows, and libnotify on Linux. (Needless to say, on Linux, X11 is
assumed, since there is no server to query the active window on Wayland.)

All symbols may not be rendered correctly above. If you see vertical hollow rectangles (or other substitute
characters), you may want to install a patched font from [Nerd Fonts](https://www.nerdfonts.com).

To actually get the custom prompt in Bash or Zsh, compile the code to obtain `custom-bash-prompt` and
`custom-zsh-prompt` (or download them from the [latest release](https://github.com/tfpf/dotfiles/releases/latest)),
copy/move them to a directory which is in `PATH`, and use them as done in [`.bash_aliases`](.bash_aliases) or
[`.zshrc`](.zshrc).
