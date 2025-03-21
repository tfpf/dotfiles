[![style](https://github.com/tfpf/dotfiles/actions/workflows/style.yml/badge.svg)](https://github.com/tfpf/dotfiles/actions/workflows/style.yml)
[![package](https://github.com/tfpf/dotfiles/actions/workflows/package.yml/badge.svg)](https://github.com/tfpf/dotfiles/actions/workflows/package.yml)

<p align="center">
  <img src="res/certified_human.svg" />
</p>

No part of the code in this repository has been written by or in consultation with artificial intelligence chatbots.
All of it is purely a product of natural intelligence (or stupidity, as the case may be).

---

These are the configuration files I use on Linux, macOS and Windows to set up a consistent development environment.

[`custom-prompt`](custom-prompt) contains code to create a prompt for Bash or Zsh with information about the current
Git repository and the current Python virtual environment, and report the running time of long commands. A session may
typically look like this.

<p align="center">
  <img src="https://github.com/user-attachments/assets/ed6d1455-bf7c-4f74-8cb4-cba68c327b6f" />
</p>

All symbols seen in the screenshot above may not be rendered correctly in the table below. If you see vertical hollow
rectangles (or other substitute characters), you may want to install a patched font from
[Nerd Fonts](https://www.nerdfonts.com).

|Component                        |Meaning                                                 |
|---------------------------------|--------------------------------------------------------|
|` workstation-a39b0e49`         |Host name                                               |
|` ~/Documents/projects/dotfiles`|Current working directory                               |
|` main`                         |Git branch currently checked out                        |
|` 0c5803a2a496 󰓼 v1.8.3`        |Commit currently checked out and associated tag (if any)|
|` 1`                            |1 file modified                                         |
|` 1`                            |1 file staged for next commit                           |
|` 1`                            |1 file not tracked                                      |
|` +1,−4`                        |Local 1 commit ahead of and 4 commits behind remote     |
|` dotfiles`                     |Current Python virtual environment                      |
|`%`                              |Default prompt symbol                                   |
|`▶%`                             |Prompt symbol in subshell                               |

If the terminal is not the active window when a long command terminates, a desktop notification will also be sent; this
is done using OSC 777 on macOS and Windows, and libnotify on Linux. (Needless to say, on Linux, X11 is assumed, since
there is no server to query the active window on Wayland.)

To actually get the custom prompt in Bash or Zsh, compile the code to obtain `custom-bash-prompt` and
`custom-zsh-prompt` (or download them from the [latest release](https://github.com/tfpf/dotfiles/releases/latest)),
copy/move them to a directory which is in `PATH`, and use them as done in [`.bash_aliases`](.bash_aliases) or
[`.zshrc`](.zshrc).
