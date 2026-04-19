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

/**
 * Read focus escape sequences (and everything else mixed with them) from
 * standard input in a non-blocking manner.
 */
class NonBlockingFocusEscapeSequenceReader
{
private:
    bool error_occurred;
    termios prev_termios, curr_termios;

public:
    NonBlockingFocusEscapeSequenceReader(void);
    ssize_t read(char[], size_t);
    ~NonBlockingFocusEscapeSequenceReader();
};

/**
 * Enable non-canonical mode and focus reporting.
 */
NonBlockingFocusEscapeSequenceReader::NonBlockingFocusEscapeSequenceReader(void) : error_occurred(false)
{
    if (tcgetattr(STDIN_FILENO, &this->prev_termios) == -1)
    {
        this->error_occurred = true;
        return;
    }
    this->curr_termios = this->prev_termios;
    this->curr_termios.c_lflag &= ~(ECHO | ICANON);
    // Block until at least as many bytes as can constitute a focus escape
    // sequence are available from standard input.
    this->curr_termios.c_cc[VMIN] = 3;
    // But not for too long.
    this->curr_termios.c_cc[VTIME] = 1;
    if (tcsetattr(STDIN_FILENO, TCSANOW, &this->curr_termios) == -1)
    {
        this->error_occurred = true;
        return;
    }
    std::clog << "\x1b\x5b?1004h";
}

/**
 * Read from standard input.
 *
 * @param buf Destination buffer.
 * @param count Maximum number of bytes to read.
 *
 * @return Number of bytes read if successful, -1 otherwise.
 */
ssize_t NonBlockingFocusEscapeSequenceReader::read(char buf[], size_t count)
{
    return ::read(STDIN_FILENO, buf, count);
}

/**
 * Disable focus reporting and non-canonical mode.
 */
NonBlockingFocusEscapeSequenceReader::~NonBlockingFocusEscapeSequenceReader()
{
    std::clog << "\x1b\x5b?1004l";
    if (this->error_occurred)
    {
        return;
    }
    tcsetattr(STDIN_FILENO, TCSANOW, &this->prev_termios);
}

bool terminal_has_focus(void)
{
    char buf[1024] = {};
    ssize_t count = NonBlockingFocusEscapeSequenceReader().read(buf, sizeof buf / sizeof *buf);
    LOG_DEBUG(logger, "Non-blocking read completed", { { "count", count }, { "buf", buf } });
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
