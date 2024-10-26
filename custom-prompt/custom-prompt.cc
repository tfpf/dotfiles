#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

namespace C
{
#include <git2.h>
#ifdef __linux__
#include <libnotify/notify.h>
#endif
}

#if defined __APPLE__
#define HOST_ICON ""
#elif defined __linux__
#define HOST_ICON ""
#elif defined _WIN32
#define HOST_ICON ""
#else
#error "unknown OS"
#endif

#if defined BASH
#define BEGIN_INVISIBLE "\x01"
#define END_INVISIBLE "\x02"
#define USER "\\u"
#define HOST "\\h"
#define DIRECTORY "\\w"
#define PROMPT_SYMBOL "\\$"
#elif defined ZSH
#define BEGIN_INVISIBLE "\x25\x7B"
#define END_INVISIBLE "\x25\x7D"
#define USER "\x25n"
#define HOST "\x25m"
#define DIRECTORY "\x25~"
#define PROMPT_SYMBOL "\x25#"
#else
#error "unknown shell"
#endif

#ifdef __MINGW32__
// Multi-byte characters are not rendered correctly. Use substitutes.
#define HISTORY_ICON "$"
#define SUCCESS_ICON "#"
#define FAILURE_ICON "#"
#else
#define HISTORY_ICON ""
#define SUCCESS_ICON ""
#define FAILURE_ICON ""
#endif

#define ESCAPE "\x1B"
#define LEFT_SQUARE_BRACKET "\x5B"
#define BACKSLASH "\x5C"
#define RIGHT_SQUARE_BRACKET "\x5D"

// Bold, bright and italic.
#define BBI_YELLOW BEGIN_INVISIBLE ESCAPE LEFT_SQUARE_BRACKET "1;3;93m" END_INVISIBLE

// Bold and bright.
#define BB_CYAN BEGIN_INVISIBLE ESCAPE LEFT_SQUARE_BRACKET "1;96m" END_INVISIBLE
#define BB_GREEN BEGIN_INVISIBLE ESCAPE LEFT_SQUARE_BRACKET "1;92m" END_INVISIBLE

// Bright.
#define B_BLUE BEGIN_INVISIBLE ESCAPE LEFT_SQUARE_BRACKET "94m" END_INVISIBLE
#define B_GREEN_RAW ESCAPE LEFT_SQUARE_BRACKET "92m"
#define B_GREY_RAW ESCAPE LEFT_SQUARE_BRACKET "90m"
#define B_RED_RAW ESCAPE LEFT_SQUARE_BRACKET "91m"

// Dark.
#define D_CYAN_RAW ESCAPE LEFT_SQUARE_BRACKET "36m"
#define D_GREEN_RAW ESCAPE LEFT_SQUARE_BRACKET "32m"
#define D_RED_RAW ESCAPE LEFT_SQUARE_BRACKET "31m"

// No formatting.
#define RESET BEGIN_INVISIBLE ESCAPE LEFT_SQUARE_BRACKET "m" END_INVISIBLE
#define RESET_RAW ESCAPE LEFT_SQUARE_BRACKET "m"

#ifndef NDEBUG
#define LOG_FMT(fmt) D_CYAN_RAW "%s" RESET_RAW ":" D_CYAN_RAW "%s" RESET_RAW ":" D_CYAN_RAW "%d" RESET_RAW " " fmt "\n"
#define LOG_DEBUG(fmt, ...) std::fprintf(stderr, LOG_FMT(fmt), __FILE__, __func__, __LINE__ __VA_OPT__(, ) __VA_ARGS__)
#else
#define LOG_DEBUG(fmt, ...)
#endif

/******************************************************************************
 * Represent an amount of time.
 *****************************************************************************/
class Interval
{
private:
    unsigned hours;
    unsigned minutes;
    unsigned seconds;
    unsigned milliseconds;

public:
    Interval(long long unsigned);
    void print_short(std::ostream&) const;
    void print_long(std::ostream&) const;
};

/******************************************************************************
 * Initialise from the given amount of time.
 *
 * @param delay Time measured in nanoseconds.
 *****************************************************************************/
Interval::Interval(long long unsigned delay)
{
    this->milliseconds = (delay /= 1000000ULL) % 1000;
    this->seconds = (delay /= 1000) % 60;
    this->minutes = (delay /= 60) % 60;
    this->hours = delay / 60;
    LOG_DEBUG("Delay is %u h %u m %u s %u ms.", this->hours, this->minutes, this->seconds, this->milliseconds);
}

/******************************************************************************
 * Output the amount of time succinctly.
 *
 * @param ostream Output stream.
 *****************************************************************************/
void Interval::print_short(std::ostream& ostream) const
{
    char fill_character = ostream.fill('0');
    if (this->hours > 0)
    {
        ostream << this->hours << ':';
    }
    ostream << std::setw(2) << this->minutes << ':';
    ostream << std::setw(2) << this->seconds << '.';
    ostream << std::setw(3) << this->milliseconds;
    ostream.fill(fill_character);
}

/******************************************************************************
 * Output the amount of time with units.
 *
 * @param ostream Output stream.
 *****************************************************************************/
void Interval::print_long(std::ostream& ostream) const
{
    if (this->hours > 0)
    {
        ostream << this->hours << " h ";
    }
    if (this->hours > 0 || this->minutes > 0)
    {
        ostream << this->minutes << " m ";
    }
    ostream << this->seconds << " s " << this->milliseconds << " ms";
}

/******************************************************************************
 * Get the ID of the currently-focused window.
 *
 * @return Active window ID. On Linux, if there is no X display running, 0 is
 * returned. Likewise, on macOS, if no topmost window is found, 0 is returned.
 *****************************************************************************/
extern "C" long long unsigned get_active_wid(void);

/******************************************************************************
 * Store information about the current Git repository.
 *****************************************************************************/
class GitRepository
{
private:
    C::git_repository* repo;
    std::filesystem::path gitdir;
    std::string ref;
    bool detached;
    bool merging, rebasing;

public:
    GitRepository(void);
    std::string reference(void);
    std::string info(void);
};

/******************************************************************************
 * Read the current Git repository.
 *****************************************************************************/
GitRepository::GitRepository(void) : merging(false), rebasing(false)
{
    C::git_libgit2_init();
    if (C::git_repository_open_ext(&this->repo, ".", 0, nullptr) != 0)
    {
        LOG_DEBUG("Failed to open.");
        return;
    }
    this->gitdir = C::git_repository_path(this->repo);
    this->ref = this->reference();
    this->detached = C::git_repository_head_detached(this->repo);
    switch (C::git_repository_state(this->repo))
    {
    case C::GIT_REPOSITORY_STATE_MERGE:
        this->merging = true;
        break;
    case C::GIT_REPOSITORY_STATE_REBASE:
    case C::GIT_REPOSITORY_STATE_REBASE_INTERACTIVE:
    case C::GIT_REPOSITORY_STATE_REBASE_MERGE:
        this->rebasing = true;
    }
}

/******************************************************************************
 * Read the reference of the current Git repository.
 *
 * @return Git reference (branch name or commit hash).
 *****************************************************************************/
std::string GitRepository::reference(void)
{
    // The head is not resolved correctly if there are no commits yet, so just
    // do this manually.
    std::ifstream head_file(this->gitdir / "HEAD");
    std::string head_contents;
    std::getline(head_file, head_contents);
    if (head_contents.rfind("ref: refs/heads/", 0) == 0)
    {
        return head_contents.substr(16);
    }
    return head_contents.substr(0, 8);
}

/******************************************************************************
 * Provide information about the current Git repository in a manner suitable to
 * display in the shell prompt.
 *
 * @return Git information.
 *****************************************************************************/
std::string GitRepository::info(void)
{
    LOG_DEBUG("ref=%s", this->ref.data());
    LOG_DEBUG("detached=%d", this->detached);
    LOG_DEBUG("merging=%d", this->merging);
    LOG_DEBUG("rebasing=%d", this->rebasing);
    return "";
}

/******************************************************************************
 * Get the current timestamp.
 *
 * @return Time in nanoseconds since a fixed but unspecified reference point.
 *****************************************************************************/
long long unsigned get_timestamp(void)
{
    std::chrono::time_point now = std::chrono::system_clock::now();
    long long unsigned ts = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
    LOG_DEBUG("Current time is %lld.%09lld.", ts / 1000000000ULL, ts % 1000000000ULL);
    return ts;
}

/******************************************************************************
 * Show a completed command using a desktop notification.
 *
 * @param last_command Most-recently run command.
 * @param exit_code Code with which the command exited.
 * @param interval Running time of the command.
 *****************************************************************************/
void notify_desktop(std::string_view const& last_command, int exit_code, Interval const& interval)
{
    std::ostringstream description_stream;
    description_stream << "exit " << exit_code << " in ";
    interval.print_long(description_stream);
    std::string description = description_stream.str();
    LOG_DEBUG("Sending notification with title '%s' and subtitle '%s'.", last_command.data(), description.data());
#if defined __APPLE__ || defined _WIN32
    // Use OSC 777, which is supported on Kitty and Wezterm, the terminals I
    // use on these systems respectively.
    std::clog << ESCAPE RIGHT_SQUARE_BRACKET "777;notify;" << last_command << ';' << description << ESCAPE BACKSLASH;
#else
    // Xfce Terminal (the best terminal) does not support OSC 777. Do it the
    // hard way.
    C::notify_init(__FILE__);
    C::NotifyNotification* notif = C::notify_notification_new(last_command.data(), description.data(), "terminal");
    C::notify_notification_show(notif, nullptr);
    // C::notify_uninit();
#endif
}

/******************************************************************************
 * Show a completed command textually.
 *
 * @param last_command Most-recently run command.
 * @param exit_code Code with which the command exited.
 * @param interval Running time of the command.
 * @param columns Width of the terminal window.
 *****************************************************************************/
void write_report(std::string_view const& last_command, int exit_code, Interval const& interval, std::size_t columns)
{
    LOG_DEBUG("Terminal width is %zu.", columns);
    std::size_t left_piece_len = columns * 3 / 8;
    std::size_t right_piece_len = left_piece_len;
    std::ostringstream report_stream;
    if (last_command.size() <= left_piece_len + right_piece_len + 5)
    {
        report_stream << D_CYAN_RAW HISTORY_ICON RESET_RAW " " << last_command;
    }
    else
    {
        LOG_DEBUG("Breaking command into pieces of lengths %zu and %zu.", left_piece_len, right_piece_len);
        report_stream << D_CYAN_RAW HISTORY_ICON RESET_RAW " " << last_command.substr(0, left_piece_len);
        report_stream << " ... " << last_command.substr(last_command.size() - right_piece_len);
    }
    if (exit_code == 0)
    {
        report_stream << " " D_GREEN_RAW SUCCESS_ICON RESET_RAW " ";
    }
    else
    {
        report_stream << " " D_RED_RAW FAILURE_ICON RESET_RAW " ";
    }
    interval.print_short(report_stream);

    // Determine the number of UTF-8 code points in the report. The C++
    // standard does not specify a UTF-8 encoding (or any encoding for that
    // matter) for all characters, but the ones for which it does are enough,
    // because the characters in the report are either those or Nerd Font
    // characters (which have specific code points), or are received as input
    // (whence their encoding doesn't matter, since they will just be output
    // without processing). Consequently, counting like this should result in
    // correct output in a UTF-8 terminal.
    std::string report = report_stream.str();
    std::size_t report_size = std::count_if(report.cbegin(), report.cend(),
        [](char const& report_char)
        {
            return (report_char & 0xC0) != 0x80;
        });
    LOG_DEBUG("Report length is %zu bytes (%zu code points).", report.size(), report_size);

    // Ensure that the text is right-aligned. Compensate for multi-byte
    // characters and non-printing sequences.
    std::size_t multi_byte_correction = report.size() - report_size;
    std::size_t constexpr non_printing_correction
        = (sizeof D_GREEN_RAW + sizeof D_RED_RAW + 2 * sizeof RESET_RAW - 4) / sizeof(char);
    std::size_t width = columns + multi_byte_correction + non_printing_correction;
    LOG_DEBUG("Padding report to %zu characters.", width);
    std::clog << '\r' << std::setw(width) << report << '\n';
}

/******************************************************************************
 * Show information about the running time of a command if it ran for long.
 *
 * @param last_command Most-recently run command.
 * @param exit_code Code with which the command exited.
 * @param delay Running time of the command in nanoseconds.
 * @param prev_active_wid ID of the focused window when the command started.
 * @param columns Width of the terminal window.
 *****************************************************************************/
void report_command_status(std::string_view& last_command, int exit_code, long long unsigned delay,
    long long unsigned prev_active_wid, std::size_t columns)
{
    LOG_DEBUG("Command '%s' exited with code %d in %llu ns.", last_command.data(), exit_code, delay);
    if (delay <= 5000000000ULL)
    {
#ifdef NDEBUG
        return;
#endif
    }

#ifdef BASH
    // Remove the initial part (index and timestamp) of the command.
    last_command.remove_prefix(last_command.find(RIGHT_SQUARE_BRACKET[0]) + 2);
#endif
    last_command.remove_suffix(last_command.size() - 1 - last_command.find_last_not_of(' '));
    LOG_DEBUG("Command length is %zu.", last_command.size());

    Interval interval(delay);
    write_report(last_command, exit_code, interval, columns);
    if (delay > 10000000000ULL)
    {
        long long unsigned curr_active_wid = get_active_wid();
        LOG_DEBUG("ID of focused window when command started was %llu.", prev_active_wid);
        LOG_DEBUG("ID of focused window when command finished is %llu.", curr_active_wid);
        if (prev_active_wid != curr_active_wid)
        {
            notify_desktop(last_command, exit_code, interval);
        }
    }
}

/******************************************************************************
 * Show the primary prompt.
 *
 * @param git_info Description of the status of the current Git repository.
 * @param shlvl Current shell level.
 *****************************************************************************/
void display_primary_prompt(std::string_view const& git_info, int shlvl)
{
    LOG_DEBUG("Current Git repository state is '%s'.", git_info.data());
    char const* venv = std::getenv("VIRTUAL_ENV_PROMPT");
    LOG_DEBUG("Current Python virtual environment is '%s'.", venv);
    std::cout << "\n┌[" BB_GREEN USER RESET " " BBI_YELLOW HOST_ICON " " HOST RESET " " BB_CYAN DIRECTORY RESET "]";
    if (!git_info.empty())
    {
        std::cout << "  " << git_info;
    }
    if (venv != nullptr)
    {
        std::cout << "  " B_BLUE << venv << RESET;
    }
    std::cout << "\n└─";
    while (--shlvl > 0)
    {
        std::cout << "▶";
    }
    std::cout << PROMPT_SYMBOL " ";
}

/******************************************************************************
 * Set the title of the current terminal window to the current directory
 * followed by the directory separator, unless the current directory is the
 * Linux/macOS root directory in which case, set the title to just a slash.
 * This should also automatically update the title of the current terminal tab.
 *
 * @param pwd Current directory.
 *****************************************************************************/
void set_terminal_title(std::string_view& pwd)
{
    LOG_DEBUG("Current directory path is '%s'.", pwd.data());
    pwd.remove_prefix(pwd.rfind('/') + 1);
    LOG_DEBUG("Current directory name is '%s'.", pwd.data());
    std::clog << ESCAPE RIGHT_SQUARE_BRACKET "0;" << pwd << '/' << ESCAPE BACKSLASH;
}

/******************************************************************************
 * Program entry point. The C++ standard forbids recursively calling `main`, so
 * the program code is written in this function instead.
 *
 * @param argc Number of command line arguments.
 * @param argv Command line arguments.
 *
 * @return Exit code.
 *****************************************************************************/
int main_internal(int const argc, char const* argv[])
{
    long long unsigned ts = get_timestamp();
    if (argc <= 1)
    {
        std::cout << ts << ' ' << get_active_wid() << '\n';
        return EXIT_SUCCESS;
    }

    // For testing. Simulate dummy arguments so that the longer branch is
    // taken. Honour the standard requirement that the argument list be
    // null-terminated.
    if (argc == 2)
    {
        char const* argv[] = { "custom-prompt", "[] last_command", "0", "0", "0", "79", "main", "1", "/", nullptr };
        int constexpr argc = sizeof argv / sizeof *argv - 1;
        return main_internal(argc, argv);
    }

    std::string_view last_command(argv[1]);
    int exit_code = std::stoi(argv[2]);
    long long unsigned delay = ts - std::stoull(argv[3]);
    long long unsigned prev_active_wid = std::stoull(argv[4]);
    std::size_t columns = std::stoull(argv[5]);
    report_command_status(last_command, exit_code, delay, prev_active_wid, columns);

    GitRepository().info();
    std::string_view git_info(argv[6]);
    int shlvl = std::stoi(argv[7]);
    display_primary_prompt(git_info, shlvl);

    std::string_view pwd(argv[8]);
    set_terminal_title(pwd);

    return EXIT_SUCCESS;
}

int main(int const argc, char const* argv[])
{
    return main_internal(argc, argv);
}
