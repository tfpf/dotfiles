###############################################################################
# User-defined functions.
###############################################################################
_after_command()
{
    local exit_code=$?
    [ -z "${__begin_window+.}" ] && return
    local last_command=$(history -n -1 2>/dev/null)
    PS1=$(custom-zsh-prompt "$last_command" $exit_code $=__begin_window $COLUMNS $PWD $SHLVL)
    unset __begin_window
}

_before_command()
{
    [ -n "${__begin_window+.}" ] && return
    __begin_window=$(custom-zsh-prompt)
}

cfs()
{
    local files=(/sys/devices/system/cpu/cpu*/cpufreq/scaling_governor)
    [ ! -f $files[1] ] && printf "CPU frequency files not found.\n" >&2 && return 1
    if [ $# -lt 1 ]
    then
        column $files
    else
        sudo tee $files <<< $1
    fi
}

e()
{
    [ -n "$VIRTUAL_ENV" ] && deactivate
    exec zsh
}

envarmunge()
{
    [ -z "$1" -o ! -d "$2" ] && return
    local name="$1"
    local value=$(realpath "$2")
    [[ :${(P)name}: != *:$value:* ]] && eval "export $name=\"$value\"\${$name:+:\$$name}"
}

h()
{
    [ ! -f "$1" ] && printf "Usage:\n  ${FUNCNAME[0]} <file>\n" >&2 && return 1
    hexdump -e '"%07.7_Ax\n"' -e '"%07.7_ax " 32/1 " %02x" "\n"' "$1" | ${=BAT_PAGER}
}

import()
{
    printf "This is Zsh. Did you mean to type this in Python?\n" >&2 && return 1
}

json.tool()
{
    python -u -c '
import json
import sys

def fmt(loader, source):
    try:
        json.dump(loader(source), ensure_ascii=False, fp=sys.stdout, indent=2, sort_keys=True)
        print()
    except json.decoder.JSONDecodeError as e:
        print(e.doc.rstrip(), file=sys.stderr)

if len(sys.argv) > 1 and sys.argv[1] == "-a":
    fmt(json.load, sys.stdin)
    raise SystemExit
for line in sys.stdin:
    fmt(json.loads, line)
    ' "$@" | bat -l json -pp --theme=TwoDark
}

o()
{
    [ ! -f "$1" ] && printf "Usage:\n  ${FUNCNAME[0]} <file>\n" >&2 && return 1
    (
        objdump -Cd "$1"
        readelf -p .rodata -x .rodata -x .data "$1" 2>/dev/null
    ) | ${=BAT_PAGER}
}

rr()
{
    case $1 in
        ("" | *[^0-9]*) local length=20;;
        (*) local length=$1;;
    esac
    for pattern in '0-9' 'A-Za-z' 'A-Za-z0-9' 'A-Za-z0-9!@#$*()'
    do
        tr -cd $pattern </dev/urandom | head -c $length
        printf "\n"
    done
}

###############################################################################
# Shell options.
###############################################################################
setopt completealiases histignoredups histignorespace ignoreeof interactive monitor promptpercent promptsubst zle
unsetopt alwayslastprompt autocd beep extendedglob nomatch notify

bindkey -e
bindkey "^[OD" backward-char
bindkey "^?" backward-delete-char  # Backspace.
bindkey "^H" backward-kill-word  # Ctrl Backspace.
bindkey "^[^?" backward-kill-word  # Alt Backspace.
bindkey "^[[1;3D" backward-word  # Alt ←.
bindkey "^[[1;5D" backward-word  # Ctrl ←.
bindkey "^A" beginning-of-line
bindkey "^[OH" beginning-of-line
bindkey "^[[1;9D" beginning-of-line  # Command ←.
bindkey "^[[H" beginning-of-line
bindkey "^[[3~" delete-char  # Delete.
bindkey "^[OB" down-history
bindkey "^[[B" down-history
bindkey "^E" end-of-line
bindkey "^[OF" end-of-line
bindkey "^[[1;9C" end-of-line  # Command →.
bindkey "^[[F" end-of-line
bindkey "^I" expand-or-complete-prefix
bindkey "^[OC" forward-char
bindkey "^[[1;3C" forward-word  # Alt →.
bindkey "^[[1;5C" forward-word  # Ctrl →.
bindkey "^[[3;3~" kill-word  # Alt Delete.
bindkey "^[[3;5~" kill-word  # Ctrl Delete.
bindkey "^[OA" up-history
bindkey "^[[A" up-history
bindkey -s "^[OM" "\n"
bindkey -s "^[Oj" "*"
bindkey -s "^[Ok" "+"
bindkey -s "^[Om" "-"
bindkey -s "^[Oo" "/"

###############################################################################
# Environment variables.
###############################################################################
export BAT_PAGER='less -iRF'
export EDITOR=vim
export GCC_COLORS='error=01;31:warning=01;35:note=01;36:caret=01;32:locus=01:quote=01:range1=32:range2=34:fixit-insert=32:fixit-delete=31:diff-filename=01:diff-hunk=32:diff-delete=31:diff-insert=32:type-diff=01;32'
export GIT_EDITOR=vim
HISTFILE=~/.zsh_history
HISTSIZE=2000
export _INKSCAPE_GC=disable
export JQ_COLORS='2;37:3;38;5;153:3;38;5;153:38;5;153:38;5;108:2:2:38;5;174'
export MANPAGER='less -i'
export MAN_POSIXLY_CORRECT=1
GIT_PS1_SHOWCOLORHINTS=1
GIT_PS1_SHOWDIRTYSTATE=1
GIT_PS1_SHOWUNTRACKEDFILES=1
export NO_AT_BRIDGE=1
export PAGER='less -i'
PS1='[%n@%m %~]%# '
PS2='──▶ '
PS3='#? '
PS4='▶ '
export PYTHONSTARTUP=$HOME/.pythonstartup.py
export PYTHON_BASIC_REPL=1
SAVEHIST=1000
export TIME_STYLE=long-iso
VIRTUAL_ENV_DISABLE_PROMPT=1
ZLE_REMOVE_SUFFIX_CHARS=
ZLE_SPACE_SUFFIX_CHARS=

export max_print_line=19999
export error_line=254
export half_error_line=238

envarmunge C_INCLUDE_PATH /usr/local/include
envarmunge CPLUS_INCLUDE_PATH /usr/local/include
envarmunge FPATH ~/.local/share/zsh-completions/completions
envarmunge LD_LIBRARY_PATH /usr/local/lib
envarmunge LD_LIBRARY_PATH /usr/local/lib64
envarmunge MANPATH /usr/share/man
envarmunge MANPATH /usr/local/man
envarmunge MANPATH /usr/local/share/man
envarmunge MANPATH ~/.local/share/man
envarmunge PATH ~/.local/bin
envarmunge PATH ~/bin
envarmunge PATH /usr/sbin
envarmunge PKG_CONFIG_PATH /usr/local/lib/pkgconfig
envarmunge PKG_CONFIG_PATH /usr/local/share/pkgconfig

export CARGO_HOME=~/.cargo
envarmunge PATH ~/.cargo/bin

export IPELATEXDIR=~/.ipe/latexrun
export IPELATEXPATH=~/.ipe/latexrun

. <(dircolors -b ~/.dircolors)

command -v lesspipe &>/dev/null && . <(SHELL=/bin/sh lesspipe)

unset CONFIG_SITE
unset GDK_SCALE
unset GIT_ASKPASS
unset SSH_ASKPASS

###############################################################################
# Built-in functions.
###############################################################################
autoload -Uz add-zsh-hook bashcompinit compinit select-word-style

# Load these and immediately execute them (Zsh does not do so automatically)
# because they help set the primary prompt.
add-zsh-hook precmd _after_command
add-zsh-hook preexec _before_command
_before_command && _after_command

# Must be called before the Bash equivalent, according the manual.
compinit
zstyle ':completion:*' file-sort name
zstyle ':completion:*' insert-tab false
zstyle ':completion:*' menu false
zstyle ':completion:*' rehash true
zstyle ':completion:*' special-dirs true
# When displaying a completion word, `PREFIX` is the string already typed at
# the prompt. Show whatever matches the base name of `PREFIX` in grey, and the
# rest of the word without any colour. Get the base name using parameter
# expansion rather than the Zsh idiom, because if `PREFIX` is, say, `/usr/`,
# the former will return the empty string, while the latter will return `usr`:
# as a result, `usr` will be coloured grey in the list of completion words
# (wherever it appears at the start of any of them), which is incorrect
# behaviour.
zstyle -e ':completion:*:default' list-colors 'BASE_OF_PREFIX=${PREFIX##*/} && reply=("${BASE_OF_PREFIX:+=(#b)($BASE_OF_PREFIX)(*)=0=1;90=0}:$LS_COLORS")'

# Needed for some programs (like pipx) which supply only Bash completion
# scripts.
bashcompinit

select-word-style bash

###############################################################################
# User-defined aliases.
###############################################################################
unalias -a

alias cpreprocess='gcc -E -x c - | command grep -Fv "#" | clang-format -style="{ColumnLimit: 119}" | bat -l c'
alias c++preprocess='gcc -E -x c++ - | command grep -Fv "#" | clang-format -style="{ColumnLimit: 119}" | bat -l c'
alias d='diff -ad -W $COLUMNS -y --suppress-common-lines'
alias g='gvim'
alias jq='jq -S'
alias less='command less -i'
alias perfstat='perf stat -e task-clock,cycles,instructions,branches,branch-misses,cache-references,cache-misses '
alias pgrep='command pgrep -il'
alias ps='command ps a -c'
alias time=$'/usr/bin/time -f "\n\e[3mReal %e s · User %U s · Kernel %S s · MRSS %M KiB · %P CPU · %c ICS · %w VCS\e[m" '
alias valgrind='command valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --verbose '
alias x='xdg-open'

alias f='watch -n 1 "command grep -F MHz /proc/cpuinfo | nl | sort -k 5 -gr"'
alias htop='command htop -d 10 -t -u $USER'
alias m='watch -d -n 1 free -ht'
alias s='watch -d -n 1 sensors'
alias top='command top -d 1 -H -u $USER'

alias l='command ls -lNX --color=auto --group-directories-first'
alias la='command ls -AhlNX --color=auto --group-directories-first'
alias ls='command ls -C --color=auto'
alias lt='command ls -AhlNrt --color=auto'

alias egrep='command grep -En --binary-files=without-match --color=auto'
alias fgrep='command grep -Fn --binary-files=without-match --color=auto'
alias grep='command grep -n --binary-files=without-match --color=auto'

if command -v batcat &>/dev/null
then
    alias bat='batcat'
    alias cat='batcat'
elif command -v bat &>/dev/null
then
    alias cat='bat'
fi

# See the Python startup file.
case $(python -m platform --terse) in
    (Linux*)
        alias p='python -B'
        alias pp='python -m IPython'
        ;;
    (macOS*)
        p()
        {
            if [ $# -ge 1 -o -n "$VIRTUAL_ENV" ]
            then
                python -B "$@"
                return
            fi
            # Homebrew-installed IPython is a separate package unassociated
            # with any Homebrew-installed Python. Hence, it must be started
            # differently.
            ipython || python -B
        }
        ;;
    (Windows*)
        alias p='python -B'
        ;;
esac
alias pip='python -m pip'
alias timeit='python -m timeit'
