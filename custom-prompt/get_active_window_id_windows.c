#ifdef _WIN32
#include <windows.h>

long long unsigned get_active_wid(void)
{
    return (long long unsigned)GetForegroundWindow();
}
#endif
