#include "focus_utils.hh"
#include "json_logger.hh"

static JSONLogger logger;

#ifdef _WIN32

#include <string_view>

#include <windows.h>

#include "focus_utils.hh"

bool terminal_has_focus(void)
{
    HWND foreground_window = GetForegroundWindow();
    TCHAR class_name[64];
    int class_name_len = GetClassName(foreground_window, class_name, sizeof class_name / sizeof *class_name);
    LOG_DEBUG(logger, "Read active window", {{"class_name_len", class_name_len}  });
    if(class_name_len == 0){
        return false;
    }
    // I use WezTerm on Windows because it supports OSC 777. Checking whether
    // a Wezterm window is active is reasonable for me.
    return std::string_view(class_name, class_name_len) == "org.wezfurlong.wezterm";
}

#else

#include <iostream>
#include <string_view>

#include <stddef.h>
#include <termios.h>
#include <unistd.h>

#define BUFSIZE 255

long long unsigned get_active_wid(void)
{
    return 0;
}

/**
 * Temporarily modify standard input to allow non-blocking reads.
 */
class NonBlockingStandardInputGuard
{
private:
    bool error_occurred;
    termios prev_termios, curr_termios;

public:
    NonBlockingStandardInputGuard(void);
    ~NonBlockingStandardInputGuard();
};

/**
 * Enable non-canonical mode.
 */
NonBlockingStandardInputGuard::NonBlockingStandardInputGuard(void) : error_occurred(false)
{
    if (tcgetattr(STDIN_FILENO, &this->prev_termios) == -1)
    {
        this->error_occurred = true;
        return;
    }
    this->curr_termios = this->prev_termios;
    this->curr_termios.c_lflag &= ~(ECHO | ICANON);
    // Block until sufficiently many bytes are available from standard input.
    this->curr_termios.c_cc[VMIN] = BUFSIZE;
    // But not for too long.
    this->curr_termios.c_cc[VTIME] = 1;
    if (tcsetattr(STDIN_FILENO, TCSANOW, &this->curr_termios) == -1)
    {
        this->error_occurred = true;
    }
}

/**
 * Disable non-canonical mode.
 */
NonBlockingStandardInputGuard::~NonBlockingStandardInputGuard()
{
    if (!this->error_occurred)
    {
    tcsetattr(STDIN_FILENO, TCSANOW, &this->prev_termios);
    }
}

bool terminal_has_focus(void)
{
    char buf[BUFSIZE];
    ssize_t count;
    {
     NonBlockingStandardInputGuard _;
     // Enable and immediately disable focus reporting.
     std::clog << "\x1b\x5b?1004h" << "\x1b\x5b?1004l";
     count = read(STDIN_FILENO, buf, sizeof buf / sizeof *buf);
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
