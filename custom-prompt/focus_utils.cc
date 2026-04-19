#ifdef _WIN32

#include "focus_utils.hh"
#include <windows.h>

long long unsigned get_active_wid(void)
{
    return static_cast<long long unsigned>(GetForegroundWindow());
}

bool terminal_has_focus(void)
{
    return false;
}

#else  ////////////////////////////////////////////////////////////////////////

#include <iostream>

#include <termios.h>
#include <unistd.h>

long long unsigned get_active_wid(void)
{
    return 0;
}

bool terminal_has_focus(void)
{
    return false;
}

#endif
