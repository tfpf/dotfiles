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
 * @param interval Time measured in easier-to-understand units.
 *****************************************************************************/
void delay_to_interval(long long unsigned delay, Interval& interval)
{
    interval.milliseconds = (delay /= 1000000ULL) % 1000;
    interval.seconds = (delay /= 1000) % 60;
    interval.minutes = (delay /= 60) % 60;
    interval.hours = delay / 60;
    LOG_DEBUG("Calculated interval is %u h %u m %u s %u ms.", interval.hours, interval.minutes, interval.seconds, interval.milliseconds);
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
void report_command_status(std::string last_command, int exit_code, long long unsigned delay, long long unsigned prev_active_wid, int columns)
{
    LOG_DEBUG("Command '%s' exited with code %d in %llu ns.", last_command.data(), exit_code, delay);
}


void display_primary_prompt(int shlvl)
{
    std::printf("$ ");
}

int main(int const argc, char const *argv[])
{
    long long unsigned ts = get_timestamp();
    if(argc <= 1)
    {
        printf("%llu %llu\n", ts, get_active_wid());
        return EXIT_SUCCESS;
    }

    // For testing. Simulate dummy arguments so that the longer branch is
    // taken.
    if (argc == 2)
    {
        // This needs to be mutable (see below), so it cannot be a string
        // literal.
        char last_command[] = "[] last_command";
        char const *argv[] = { "custom-prompt", last_command, "0", "0", "0", "79", "1"};
        return main(7, argv);
    }

    // The function which receives the first argument may modify it. (This is
    // allowed in C++.) That's why the command line arguments were not declared
    // read-only.
    char const *last_command = argv[1];
    int exit_code = std::stoi(argv[2]);
    long long unsigned delay = ts - std::stoull(argv[3]);
    long long unsigned prev_active_wid = std::stoull(argv[4]);
    int columns = std::stoi(argv[5]);
    report_command_status(last_command, exit_code, delay, prev_active_wid, columns);

    int shlvl = std::stoi(argv[6]);
    display_primary_prompt(shlvl);

    return EXIT_SUCCESS;
}
