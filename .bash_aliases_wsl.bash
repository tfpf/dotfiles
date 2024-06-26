# Setup for a virtual display using VcXsrv to run GUI apps. You may want to
# install `x11-xserver-utils`, `dconf-editor` and `dbus-x11`, and create
# the file `$HOME/.config/dconf/user` to avoid getting warnings.
if command grep -Fq WSL2 /proc/version
then
    export DISPLAY=$(command grep -F -m 1 nameserver /etc/resolv.conf | awk '{print $2}'):0
else
    export DISPLAY=localhost:0.0
fi
export LIBGL_ALWAYS_INDIRECT=1

# Run a PowerShell script without changing the global execution policy.
alias psh='powershell.exe -ExecutionPolicy Bypass'

# Compile programs written in C#.
alias csc='/mnt/c/Windows/Microsoft.NET/Framework64/v4.0.30319/csc.exe'

# Create the virtual display. VcXsrv should be installed.
vcx()
{
    local vcxsrvpath='/mnt/c/Program Files/VcXsrv/vcxsrv.exe'
    local vcxsrvname=${vcxsrvpath##*/}

    # If VcXsrv is started in the background, it writes messages to
    # standard error, and an entry corresponding to it remains in the Linux
    # process list though it is actually a Windows process. Hence, start it
    # in a subshell, suppress its output, and terminate the Linux process.
    # (This does not affect the Windows process.) This pattern may be used
    # in a few more functions below.
    ("$vcxsrvpath" -ac -clipboard -multiwindow -wgl & sleep 1 && pkill "$vcxsrvname" &) &>/dev/null
}

# GVIM for Windows.
unalias g
g()
{
    [ ! -f "$1" ] && printf "Usage:\n  ${FUNCNAME[0]} <file>\n" >&2 && return 1

    # See https://tuxproject.de/projects/vim/ for 64-bit Windows binaries.
    local gvimpath='/mnt/c/Program Files (x86)/Vim/vim90/gvim.exe'
    local gvimname=${gvimpath##*/}
    local filedir=$(dirname "$1")
    local filename=${1##*/}
    (cd "$filedir" && "$gvimpath" "$filename" & sleep 1 && pkill "$gvimname" &) &>/dev/null
}

# Open a file or link.
unalias x
x()
{
    # Windows Explorer can open WSL directories, but the command must be
    # invoked after navigating to the target directory to avoid problems
    # like a tilde getting interpreted as the Windows home folder instead
    # of the Linux home directory. To avoid changing `OLDPWD`, do this in a
    # subshell.
    if [ -d "$1" ]
    then
        (
            cd "$1"
            explorer.exe .
        )
    else
        xdg-open "$1"
    fi
}
