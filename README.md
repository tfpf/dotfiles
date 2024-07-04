I use these files on Linux, macOS and Windows.

[![style](https://github.com/tfpf/dotfiles/actions/workflows/style.yml/badge.svg)](https://github.com/tfpf/dotfiles/actions/workflows/style.yml)
[![package](https://github.com/tfpf/dotfiles/actions/workflows/package.yml/badge.svg)](https://github.com/tfpf/dotfiles/actions/workflows/package.yml)

[`custom-prompt`](custom-prompt) contains code to create a prompt for Bash or Zsh with information about the current
Git repository (from `__git_ps1`) and the current Python virtual environment. The number of right-pointing triangles
just before the prompt symbol is one less than the current shell level.

<pre>
┌[<span style="color:#8AE234">tfpf</span> <span style="color:#FCE94F"> Alpine</span> <span style="color:#34E2E2">~/Documents/projects/dotfiles</span>]  <span style="color:#4E9A06">main</span> <span style="color:#CC0000">*</span>  <span style="color:#739FCF">dotfiles</span>
└─▶%
</pre>

All symbols may not be rendered correctly above. If you see vertical hollow rectangles (or other substitute
characters), you may want to grab a patched font from [Nerd Fonts](https://www.nerdfonts.com).

To actually get the custom prompt in Bash or Zsh, compile the code to obtain `custom-bash-prompt` and
`custom-zsh-prompt` (or grab them from the [latest release](https://github.com/tfpf/dotfiles/releases/latest)), and use
them as done in [`.bash_aliases`](.bash_aliases) or [`.zshrc`](.zshrc).
