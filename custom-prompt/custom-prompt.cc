#include <iostream>
#include<iomanip>
#include<sstream>
#include <cstddef>
#include<chrono>
#include <string>
#include <cstdlib>
#include <cstdio>

#ifdef __linux__
namespace C{
#include <libnotify/notify.h>
}
#endif

struct Interval
{
    unsigned hours;
    unsigned minutes;
    unsigned seconds;
    unsigned milliseconds;

    friend std::ostringstream& operator<<(std::ostringstream& stream, Interval const& interval){
        stream << std::setfill('0');
        if(interval.hours > 0){
            stream << std::setw(2) << interval.hours << ':' ;
        }
        stream<< std::setw(2) << interval.minutes << ':';
        stream << std::setw(2) << interval.seconds << '.';
        stream << std::setw(3) << interval.milliseconds;
        stream << std::setfill(' ');
        return stream;
    }
};

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
#define BEGIN_INVISIBLE "%%\x7B"
#define END_INVISIBLE "%%\x7D"
#define USER "%%n"
#define HOST "%%m"
#define DIRECTORY "%%~"
#define PROMPT_SYMBOL "%%#"
#else
#error "unknown shell"
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

// No formatting.
#define RESET BEGIN_INVISIBLE ESCAPE LEFT_SQUARE_BRACKET "m" END_INVISIBLE
#define RESET_RAW ESCAPE LEFT_SQUARE_BRACKET "m"

#ifndef NDEBUG
#define LOG_FMT(fmt) D_CYAN_RAW "%s" RESET_RAW ":" D_CYAN_RAW "%s" RESET_RAW ":" D_CYAN_RAW "%d" RESET_RAW " " fmt "\n"
#define LOG_DEBUG(fmt, ...) std::fprintf(stderr, LOG_FMT(fmt), __FILE__, __func__, __LINE__ __VA_OPT__(,) __VA_ARGS__);
#else
#define LOG_DEBUG(fmt, ...)
#endif

/******************************************************************************
 * Get the ID of the currently-focused window.
 *
 * @return Active window ID.
 *****************************************************************************/
extern "C" long long unsigned get_active_wid(void);


/******************************************************************************
 * Get the current timestamp.
 *
 * @return Time in nanoseconds since a fixed but unspecified reference point.
 *****************************************************************************/
long long unsigned get_timestamp(void)
{
    auto now = std::chrono::system_clock::now();
    long long unsigned ts = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
    LOG_DEBUG("Current time is %lld.%09lld.", ts / 1000000000ULL, ts % 1000000000ULL);
    return ts;
}

/******************************************************************************
 * Represent an amount of time in human-readable form. Write it into the
 * provided object.
 *
 * @param delay Time measured in nanoseconds.
 *
 * @return Time measured in easier-to-understand units.
 *****************************************************************************/
Interval delay_to_interval(long long unsigned delay)
{
    Interval interval;
    interval.milliseconds = (delay /= 1000000ULL) % 1000;
    interval.seconds = (delay /= 1000) % 60;
    interval.minutes = (delay /= 60) % 60;
    interval.hours = delay / 60;
    LOG_DEBUG("Calculated interval is %u h %u m %u s %u ms.", interval.hours, interval.minutes, interval.seconds, interval.milliseconds);
    return interval;
}

/******************************************************************************
 * Show a completed command using a desktop notification.
 *
 * @param last_command Most-recently run command.
 * @param exit_code Code with which the command exited.
 * @param interval Running time of the command.
 *****************************************************************************/
void notify_desktop(std::string_view const&last_command, int exit_code, Interval const &interval)
{
//     std::ostringstream description_stream;
// 
//     static char description[64];
//     char *description_ptr = description;
//     description_ptr += sprintf(description_ptr, "exit %d in ", exit_code);
//     if (interval->hours > 0)
//     {
//         description_ptr += sprintf(description_ptr, "%u h ", interval->hours);
//     }
//     if (interval->hours > 0 || interval->minutes > 0)
//     {
//         description_ptr += sprintf(description_ptr, "%u m ", interval->minutes);
//     }
//     description_ptr += sprintf(description_ptr, "%u s %u ms", interval->seconds, interval->milliseconds);
//     LOG_DEBUG("Sending notification with title '%s' and subtitle '%s'.", last_command, description);
// #if defined __APPLE__ || defined _WIN32
//     // Use OSC 777, which is supported on Kitty and Wezterm, the terminals I
//     // use on these systems respectively.
//     fprintf(stderr, ESCAPE RIGHT_SQUARE_BRACKET "777;notify;%s;%s" ESCAPE BACKSLASH, last_command, description);
// #else
//     // Xfce Terminal (the best terminal) does not support OSC 777. Do it the
//     // hard way.
//     notify_init(__FILE__);
//     NotifyNotification *notification = notify_notification_new(last_command, description, "terminal");
//     notify_notification_show(notification, NULL);
//     // notify_uninit();
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
    std::size_t last_command_size = last_command.size();
    std::ostringstream report_stream;
    if (last_command_size <= left_piece_len + right_piece_len + 5)
    {
        report_stream << " " << last_command;
    }
    else
    {
        LOG_DEBUG("Breaking command into pieces of lengths %zu and %zu.", left_piece_len, right_piece_len);
        report_stream << " " << last_command.substr(0, left_piece_len);
        report_stream << " ... " << last_command.substr(last_command_size - right_piece_len);
    }
    if (exit_code == 0)
    {
        report_stream << " " B_GREEN_RAW "" RESET_RAW " ";
    }
    else
    {
        report_stream << " " B_RED_RAW "" RESET_RAW " ";
    }
    report_stream << interval;

    // Ensure that the text is right-aligned. Since there are non-printable
    // characters in it, compensate for the width.
    std::size_t width = columns + 12;
    std::string report = report_stream.str();
    LOG_DEBUG("Report length is %zu.", report.size());
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
void report_command_status(std::string_view& last_command, int exit_code, long long unsigned delay, long long unsigned prev_active_wid, std::size_t columns)
{
    LOG_DEBUG("Command '%s' exited with code %d in %llu ns.", last_command.data(), exit_code, delay);
    if(delay <= 5000000000ULL){
#ifdef NDEBUG
        return;
#endif
    }

    Interval interval = delay_to_interval(delay);

#ifdef BASH
    // Remove the initial part (index and timestamp) of the command.
    last_command = last_command.substr(last_command.find(RIGHT_SQUARE_BRACKET[0]) + 2);
#endif
    last_command.remove_suffix(last_command.size() - 1 - last_command.find_last_not_of(' '));
    LOG_DEBUG("Command length is %zu.", last_command.size());

    write_report(last_command, exit_code, interval, columns);
    if(delay > 10000000000ULL){
        long long unsigned curr_active_wid = get_active_wid();
        LOG_DEBUG("ID of focused window when command started was %llu.", prev_active_wid);
        LOG_DEBUG("ID of focused window when command finished is %llu.", curr_active_wid);
        if (prev_active_wid != curr_active_wid)
        {
            notify_desktop(last_command, exit_code, interval);
        }
    }
}


void display_primary_prompt(int shlvl)
{
    std::printf("%d $ ", shlvl);
}

int main(int const argc, char const *argv[])
{
    long long unsigned ts = get_timestamp();
    if(argc <= 1)
    {
        std::printf("%llu %llu\n", ts, get_active_wid());
        return EXIT_SUCCESS;
    }

    // For testing. Simulate dummy arguments so that the longer branch is
    // taken.
    if (argc == 2)
    {
        return main(7, (char const*[]){ "custom-prompt", "[] last_command", "0", "0", "0", "79", "1"});
    }

    std::string_view last_command(argv[1]);
    int exit_code = std::stoi(argv[2]);
    long long unsigned delay = ts - std::stoull(argv[3]);
    long long unsigned prev_active_wid = std::stoull(argv[4]);
    std::size_t columns = std::stoull(argv[5]);
    report_command_status(last_command, exit_code, delay, prev_active_wid, columns);

    int shlvl = std::stoi(argv[6]);
    display_primary_prompt(shlvl);

    return EXIT_SUCCESS;
}