#ifdef _WIN32

#include <windows.h>

#include "focus_utils.hh"

long long unsigned get_active_wid(void)
{
    return static_cast<long long unsigned>(GetForegroundWindow());
}

bool terminal_has_focus(void)
{
    return false;
}

#else  ////////////////////////////////////////////////////////////////////////

#include <string_view>

#include <stddef.h>
#include <termios.h>
#include <unistd.h>

#include "focus_utils.hh"

long long unsigned get_active_wid(void)
{
    return 0;
}

class NonBlockingStandardInputReader
{
private:
    bool error_occurred;
    termios prev_termios, curr_termios;

public:
    NonBlockingStandardInputReader(void);
    ssize_t read(char[], size_t);
    ~NonBlockingStandardInputReader();
};

NonBlockingStandardInputReader::NonBlockingStandardInputReader(void) : error_occurred(false)
{
    if (tcgetattr(STDIN_FILENO, &this->prev_termios) == -1)
    {
        this->error_occurred = true;
    }
    this->curr_termios = this->prev_termios;
    this->curr_termios.c_lflag &= ~(ECHO | ICANON);
    this->curr_termios.c_cc[VMIN] = 0;
    this->curr_termios.c_cc[VTIME] = 0;
    if (tcsetattr(STDIN_FILENO, TCSANOW, &this->curr_termios) == -1)
    {
        this->error_occurred = true;
    }
}

ssize_t NonBlockingStandardInputReader::read(char buf[], size_t count)
{
    return ::read(STDIN_FILENO, buf, count);
}

NonBlockingStandardInputReader::~NonBlockingStandardInputReader()
{
    if (this->error_occurred)
    {
        return;
    }
    tcsetattr(STDIN_FILENO, TCSANOW, &this->prev_termios);
}

bool terminal_has_focus(void)
{
    char buf[1024] = {};
    ssize_t cnt = NonBlockingStandardInputReader().read(buf, sizeof buf / sizeof *buf);
    if (cnt <= 0)
    {
        return false;
    }

    return false;
}

#endif
