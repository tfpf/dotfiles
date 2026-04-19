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

#include <iostream>
#include <string_view>
#include <chrono>
#include <thread>

#include <stddef.h>
#include <termios.h>
#include <unistd.h>

#include "focus_utils.hh"
#include "json_logger.hh"

static JSONLogger logger;

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
        return;
    }
    this->curr_termios = this->prev_termios;
    this->curr_termios.c_lflag &= ~(ECHO | ICANON);
    this->curr_termios.c_cc[VMIN] = 0;
    this->curr_termios.c_cc[VTIME] = 0;
    if (tcsetattr(STDIN_FILENO, TCSANOW, &this->curr_termios) == -1)
    {
        this->error_occurred = true;
        return;
    }
    LOG_DEBUG(logger, "Enabling focus mode" );
    std::clog << "\x1b\x5b?1004h";
}

ssize_t NonBlockingStandardInputReader::read(char buf[], size_t count)
{
    LOG_DEBUG(logger, "Reading focus sequence" );
    std::this_thread::sleep_for(std::chrono::seconds(5));
    return ::read(STDIN_FILENO, buf, count);
}

NonBlockingStandardInputReader::~NonBlockingStandardInputReader()
{
    if (this->error_occurred)
    {
        return;
    }
    tcsetattr(STDIN_FILENO, TCSANOW, &this->prev_termios);
    LOG_DEBUG(logger, "Disabling focus mode" );
    std::clog << "\x1b\x5b?1004l";
}

bool terminal_has_focus(void)
{
    char buf[1024] = {};
    ssize_t count = NonBlockingStandardInputReader().read(buf, sizeof buf / sizeof *buf);
    LOG_DEBUG(logger, "Read", {{"count", count}});
    if (count <= 0)
    {
        return false;
    }

    std::string_view buf_view(buf, count);
    size_t focus_in_seq_pos = buf_view.rfind("\x1b\x5bI");
    if (focus_in_seq_pos == std::string_view::npos)
    {
        return false;
    }
    size_t focus_out_seq_pos = buf_view.rfind("\x1b\x5bO");
    if (focus_out_seq_pos == std::string_view::npos)
    {
        return true;
    }
    return focus_in_seq_pos > focus_out_seq_pos;
}

#endif
