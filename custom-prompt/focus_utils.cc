#include "focus_utils.hh"
#include "json_logger.hh"

static JSONLogger logger;

#ifdef _WIN32

#include <tchar.h>
#include <windows.h>

bool terminal_has_focus(void)
{
    HWND foreground_window = GetForegroundWindow();
    TCHAR class_name[64];
    int class_name_len = GetClassName(foreground_window, class_name, sizeof class_name / sizeof *class_name);
    LOG_DEBUG(logger, "Read active window", { { "class_name_len", class_name_len } });
    if (class_name_len == 0)
    {
        return false;
    }
    // I use WezTerm on Windows because it supports OSC 777; I never open more
    // than one window. Hence, checking whether a Wezterm window is active is
    // sufficient for me.
    return _tcscmp(class_name, _T("org.wezfurlong.wezterm")) == 0;
}

#else

#include <iostream>
#include <string_view>

#include <stddef.h>
#include <termios.h>
#include <unistd.h>

/**
 * Temporarily modify standard input to allow non-blocking reads.
 */
class NonBlockingStandardInputGuard
{
private:
    bool m_error_occurred;
    termios prev_termios, curr_termios;

public:
    NonBlockingStandardInputGuard(void);
    bool error_occurred(void) const;
    ~NonBlockingStandardInputGuard();
};

/**
 * Enable non-canonical mode.
 */
NonBlockingStandardInputGuard::NonBlockingStandardInputGuard(void) : m_error_occurred(false)
{
    if (tcgetattr(STDIN_FILENO, &this->prev_termios) == -1)
    {
        this->m_error_occurred = true;
        return;
    }
    this->curr_termios = this->prev_termios;
    this->curr_termios.c_lflag &= ~(ECHO | ICANON);
    // Set a timeout on standard input reads.
    this->curr_termios.c_cc[VMIN] = 0;
    this->curr_termios.c_cc[VTIME] = 1;
    if (tcsetattr(STDIN_FILENO, TCSANOW, &this->curr_termios) == -1)
    {
        this->m_error_occurred = true;
    }
}

/**
 * Check whether an error occurred while trying to get or set terminal
 * attributes.
 *
 * @return Error indicator.
 */
bool NonBlockingStandardInputGuard::error_occurred(void) const
{
    return this->m_error_occurred;
}

/**
 * Disable non-canonical mode.
 */
NonBlockingStandardInputGuard::~NonBlockingStandardInputGuard()
{
    if (!this->m_error_occurred)
    {
        tcsetattr(STDIN_FILENO, TCSANOW, &this->prev_termios);
    }
}

bool terminal_has_focus(void)
{
    char buf[1024];
    ssize_t count;
    {
        NonBlockingStandardInputGuard guard;
        if (guard.error_occurred())
        {
            return false;
        }
        // Consume standard input so that anything entered previously does not
        // get read instead of the focus escape sequence.
        while (true)
        {
            count = read(STDIN_FILENO, buf, sizeof buf / sizeof *buf);
            if (count < 0)
            {
                return false;
            }
            if (count == 0)
            {
                break;
            }
        }
        // Enable focus reporting.
        std::clog << "\x1b\x5b?1004h";
        count = read(STDIN_FILENO, buf, sizeof buf / sizeof *buf);
        // Disable focus reporting. If it is disabled immediately after
        // enabling it instead of here, the terminal may never send any focus
        // escape sequences. There is a small chance that the sequences get
        // written to standard input before focus reporting gets disabled but
        // after the previous sequences have been read. It should cause no
        // damage, so I'll allow it.
        std::clog << "\x1b\x5b?1004l";
    }
    LOG_DEBUG(logger, "Read non-blocking standard input", { { "count", count } });
    if (count <= 0)
    {
        return false;
    }
    std::string_view buf_view(buf, count);
    size_t focus_out_seq_pos = buf_view.rfind("\x1b\x5bO");
    if (focus_out_seq_pos == std::string_view::npos)
    {
        return true;
    }
    size_t focus_in_seq_pos = buf_view.rfind("\x1b\x5bI");
    if (focus_in_seq_pos == std::string_view::npos)
    {
        return false;
    }
    return focus_out_seq_pos < focus_in_seq_pos;
}

#endif
