unalias -a

alias bye='true && exit'
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

alias f='watch -n 1 "command grep -F MHz /proc/cpuinfo | nl -n rz -w 2 | sort -k 5 -gr | sed s/^0/\ /g"'
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

alias L="$HOME/.bash_hacks.py L"
alias P="$HOME/.bash_hacks.py P"
alias T="$HOME/.bash_hacks.py T"

# Control CPU frequency scaling.
cfs()
{
    local files=(/sys/devices/system/cpu/cpu*/cpufreq/scaling_governor)
    [ ! -f ${files[0]} ] && printf "CPU frequency files not found.\n" >&2 && return 1
    if [ $# -lt 1 ]
    then
        column ${files[@]}
    else
        sudo tee ${files[@]} <<< $1
    fi
}

# Restart the shell. Exit from any Python virtual environments before doing so.
e()
{
    [ -n "$VIRTUAL_ENV" ] && deactivate
    exec bash
}

# View object files.
o()
{
    [ ! -f "$1" ] && printf "Usage:\n  ${FUNCNAME[0]} <file>\n" >&2 && return 1
    (
        objdump -Cd "$1"
        readelf -p .rodata -x .rodata -x .data "$1" 2>/dev/null
    ) | $BAT_PAGER
}

# View raw data.
h()
{
    [ ! -f "$1" ] && printf "Usage:\n  ${FUNCNAME[0]} <file>\n" >&2 && return 1
    hexdump -e '"%07.7_Ax\n"' -e '"%07.7_ax " 32/1 " %02x" "\n"' "$1" | $BAT_PAGER
}

# Preprocess C or C++ source code.
c()
{
    [ ! -f "$1" ] && printf "Usage:\n  ${FUNCNAME[0]} <file>\n" >&2 && return 1
    if [ "$2" == ++ ]
    then
        local c=g++
        local l=c++
    else
        local c=gcc
        local l=c
    fi
    clang-format <($c -E "$1" | command grep -Fv '#') | bat -l $l --file-name "$1"
}

# This is executed only if Bash is running on WSL (Windows Subsystem for
# Linux).
command grep -Fiq microsoft /proc/version && . $HOME/.bash_aliases_wsl.bash

# Pre-command for command timing. It will be called just before any command is
# executed.
_before_command()
{
    [ -n "${__begin_window+.}" ] && return
    __begin_window=$(custom-bash-prompt)
}

# Post-command for command timing. It will be called just before the prompt is
# displayed (i.e. just after any command is executed).
_after_command()
{
    local exit_code=$?
    [ -z "${__begin_window+.}" ] && return
    local last_command=$(history 1)
    PS1=$(custom-bash-prompt "$last_command" $exit_code $__begin_window $COLUMNS "$PWD" $SHLVL)
    unset __begin_window
}

trap _before_command DEBUG
PROMPT_COMMAND=_after_command

# PDF optimiser. This requires that Ghostscript be installed.
pdfopt()
{
    [ ! -f "$1" ] && printf "Usage:\n  ${FUNCNAME[0]} input_file.pdf output_file.pdf [resolution]\n" >&2 && return 1
    [ $# -ge 3 ] && local opt_level=$3 || local opt_level=72
    gs -sDEVICE=pdfwrite -dCompatibilityLevel=1.4 -dPDFSETTINGS=/prepress     \
       -dNOPAUSE -dQUIET -dBATCH                                              \
       -sOutputFile="$2"                                                      \
       -dDownsampleColorImages=true -dColorImageResolution=$opt_level         \
       -dDownsampleGrayImages=true  -dGrayImageResolution=$opt_level          \
       -dDownsampleMonoImages=true  -dMonoImageResolution=$opt_level          \
       "$1"
}

# Random string generator.
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

# This program (part of the ImageMagick suite) is just schmuck bait. I've
# entered this at the Bash REPL instead of the Python REPL several times, and
# got scared because the computer seemed to freeze. No more.
import()
{
    printf "This is Bash. Did you mean to type this in Python?\n" >&2 && return 1
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
