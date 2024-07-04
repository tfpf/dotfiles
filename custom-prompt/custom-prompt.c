#include <ctype.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef __linux__
#include <libnotify/notify.h>
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
#define LOG_DEBUG(fmt, ...) log_debug(__FILE__, __func__, __LINE__, fmt __VA_OPT__(, ) __VA_ARGS__)
#else
#define LOG_DEBUG(fmt, ...)
#endif

/******************************************************************************
 * Write a debugging message.
 *
 * @param file_name
 * @param function_name
 * @param line_number
 * @param fmt Format string. Arguments after it will be used to format it.
 *****************************************************************************/
void log_debug(char const *file_name, char const *function_name, int line_number, char const *fmt, ...)
{
    time_t now = time(NULL);
    static char local_iso[64];
    strftime(local_iso, sizeof local_iso / sizeof *local_iso, "%FT%T%z", localtime(&now));
    fprintf(stderr, B_GREY_RAW "%s" RESET_RAW " ", local_iso);
    fprintf(stderr, D_CYAN_RAW "%s" RESET_RAW ":", file_name);
    fprintf(stderr, D_CYAN_RAW "%s" RESET_RAW ":", function_name);
    fprintf(stderr, D_CYAN_RAW "%d" RESET_RAW " ", line_number);
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    // va_end(ap);
    fprintf(stderr, "\n");
}

/******************************************************************************
 * Get the ID of the currently-focused window.
 *
 * @return Active window ID.
 *****************************************************************************/
long long unsigned get_active_wid(void);

/******************************************************************************
 * Get the current timestamp.
 *
 * @return Time in nanoseconds since a fixed but unspecified reference point.
 *****************************************************************************/
long long unsigned get_timestamp(void)
{
    struct timespec now;
    timespec_get(&now, TIME_UTC);
    LOG_DEBUG("Current time is %lld.%09ld.", (long long)now.tv_sec, now.tv_nsec);
    return now.tv_sec * 1000000000ULL + now.tv_nsec;
}

/******************************************************************************
 * Represent an amount of time in human-readable form.
 *
 * @param delay Time measured in nanoseconds.
 * @param interval Time measured in easier-to-understand units.
 *****************************************************************************/
void delay_to_interval(long long unsigned delay, struct Interval *interval)
{
    interval->milliseconds = (delay /= 1000000ULL) % 1000;
    interval->seconds = (delay /= 1000) % 60;
    interval->minutes = (delay /= 60) % 60;
    interval->hours = delay / 60;
    LOG_DEBUG("Calculated interval is %u h %u m %u s %u ms.", interval->hours, interval->minutes, interval->seconds,
        interval->milliseconds);
}

/******************************************************************************
 * Show a completed command using a desktop notification.
 *
 * @param last_command Most-recently run command.
 * @param exit_code Code with which the command exited.
 * @param interval Running time of the command.
 *****************************************************************************/
void notify_desktop(char const *last_command, int exit_code, struct Interval const *interval)
{
    static char description[64];
    char *description_ptr = description;
    description_ptr += sprintf(description_ptr, "exit %d in ", exit_code);
    if (interval->hours > 0)
    {
        description_ptr += sprintf(description_ptr, "%u h ", interval->hours);
    }
    if (interval->hours > 0 || interval->minutes > 0)
    {
        description_ptr += sprintf(description_ptr, "%u m ", interval->minutes);
    }
    description_ptr += sprintf(description_ptr, "%u s %u ms", interval->seconds, interval->milliseconds);
    LOG_DEBUG("Sending notification with title '%s' and subtitle '%s'.", last_command, description);
#if defined __APPLE__ || defined _WIN32
    // Use OSC 777, which is supported on Kitty and Wezterm, the terminals I
    // use on these systems respectively.
    fprintf(stderr, ESCAPE RIGHT_SQUARE_BRACKET "777;notify;%s;%s" ESCAPE BACKSLASH, last_command, description);
#else
    // Xfce Terminal (the best terminal) does not support OSC 777. Do it the
    // hard way.
    notify_init(__FILE__);
    NotifyNotification *notification = notify_notification_new(last_command, description, "terminal");
    notify_notification_show(notification, NULL);
    // notify_uninit();
#endif
}

/******************************************************************************
 * Show a completed command textually.
 *
 * @param last_command Most-recently run command.
 * @param last_command_len Length of the command.
 * @param exit_code Code with which the command exited.
 * @param interval Running time of the command.
 * @param columns Width of the terminal window.
 *****************************************************************************/
void write_report(
    char const *last_command, size_t last_command_len, int exit_code, struct Interval const *interval, int columns)
{
    char *report = malloc((last_command_len + 64) * sizeof *report);
    char *report_ptr = report;

    LOG_DEBUG("Terminal width is %d.", columns);
    int left_piece_len = columns * 3 / 8;
    int right_piece_len = left_piece_len;
    if (last_command_len <= (size_t)(left_piece_len + right_piece_len) + 5)
    {
        report_ptr += sprintf(report_ptr, " %s ", last_command);
    }
    else
    {
        LOG_DEBUG("Breaking command into pieces of lengths %d and %d.", left_piece_len, right_piece_len);
        report_ptr += sprintf(report_ptr, " %.*s ... ", left_piece_len, last_command);
        report_ptr += sprintf(report_ptr, "%s ", last_command + last_command_len - right_piece_len);
    }
    if (exit_code == 0)
    {
        report_ptr += sprintf(report_ptr, B_GREEN_RAW "" RESET_RAW " ");
    }
    else
    {
        report_ptr += sprintf(report_ptr, B_RED_RAW "" RESET_RAW " ");
    }
    if (interval->hours > 0)
    {
        report_ptr += sprintf(report_ptr, "%02u:", interval->hours);
    }
    report_ptr += sprintf(report_ptr, "%02u:%02u.%03u", interval->minutes, interval->seconds, interval->milliseconds);

    // Ensure that the text is right-aligned. Since there are non-printable
    // characters in the string, compensate for the width.
    int width = columns + 12;
    LOG_DEBUG("Report length is %ld.", report_ptr - report);
    LOG_DEBUG("Padding report to %d characters.", width);
    fprintf(stderr, "\r%*s\n", width, report);
    // free(report);
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
void report_command_status(
    char *last_command, int exit_code, long long unsigned delay, long long unsigned prev_active_wid, int columns)
{
    LOG_DEBUG("Command '%s' exited with code %d in %llu ns.", last_command, exit_code, delay);
    if (delay <= 5000000000ULL)
    {
#ifdef NDEBUG
        return;
#endif
    }

    struct Interval interval;
    delay_to_interval(delay, &interval);

#ifdef BASH
    // Remove the initial part (index and timestamp) of the command.
    last_command = strchr(last_command, RIGHT_SQUARE_BRACKET[0]) + 2;
#endif
    // Remove trailing whitespace characters, if any. Then allocate enough
    // space to write what remains and some additional information.
    size_t last_command_len = strlen(last_command);
    while (isspace(last_command[--last_command_len]) != 0)
    {
    }
    last_command[++last_command_len] = '\0';
    LOG_DEBUG("Command length is %zu.", last_command_len);

    write_report(last_command, last_command_len, exit_code, &interval, columns);
    if (delay > 10000000000ULL)
    {
        long long unsigned curr_active_wid = get_active_wid();
        LOG_DEBUG("ID of focused window when command started was %llu.", prev_active_wid);
        LOG_DEBUG("ID of focused window when command finished is %llu.", curr_active_wid);
        if (prev_active_wid != curr_active_wid)
        {
            notify_desktop(last_command, exit_code, &interval);
        }
    }
}

/******************************************************************************
 * Update the title of the current terminal window. This should also
 * automatically update the title of the current terminal tab.
 *
 * @param pwd Current directory.
 *****************************************************************************/
void update_terminal_title(char const *pwd)
{
    LOG_DEBUG("Current directory is '%s'.", pwd);
    char const *short_pwd = strrchr(pwd, '/') + 1;
    LOG_DEBUG("Setting terminal window title to '%s/'.", short_pwd);
    fprintf(stderr, ESCAPE RIGHT_SQUARE_BRACKET "0;%s/" ESCAPE BACKSLASH, short_pwd);
}

/******************************************************************************
 * Show the primary prompt.
 *
 * @param git_info Description of the status of the current Git repository.
 * @param shlvl Current shell level.
 *****************************************************************************/
void display_primary_prompt(char const *git_info, int shlvl)
{
    char const *venv = getenv("VIRTUAL_ENV_PROMPT");
    LOG_DEBUG("Current Python virtual environment is '%s'.", venv);
    printf("\n┌[" BB_GREEN USER RESET " " BBI_YELLOW HOST_ICON " " HOST RESET " " BB_CYAN DIRECTORY RESET "]");
    if (git_info[0] != '\0')
    {
        printf("  %s", git_info);
    }
    if (venv != NULL)
    {
        printf("  " B_BLUE "%s" RESET, venv);
    }
    printf("\n└─");
    while (--shlvl > 0)
    {
        printf("▶");
    }
    printf(PROMPT_SYMBOL " ");
}

int main(int const argc, char const *argv[])
{
    long long unsigned ts = get_timestamp();
    if (argc <= 1)
    {
        printf("%llu %llu\n", ts, get_active_wid());
        return EXIT_SUCCESS;
    }

    // For testing. Simulate dummy arguments so that the longer branch is
    // taken.
    if (argc == 2 && argv[1][0] == '\0')
    {
        // This needs to be mutable (see below), so it cannot be a string
        // literal.
        char last_command[] = "[] last_command";
        return main(9, (char const *[]) { "custom-prompt", last_command, "0", "0", "0", "79", "git_info", "1", "/" });
    }

    // Mark the first argument as mutable (this is allowed in C) to avoid
    // copying it in the function which receives it.
    char *last_command = (char *)argv[1];
    int exit_code = strtol(argv[2], NULL, 10);
    long long unsigned delay = ts - strtoll(argv[3], NULL, 10);
    long long unsigned prev_active_wid = strtoull(argv[4], NULL, 10);
    int columns = strtol(argv[5], NULL, 10);
    char const *git_info = argv[6];
    int shlvl = strtol(argv[7], NULL, 10);
    char const *pwd = argv[8];

    report_command_status(last_command, exit_code, delay, prev_active_wid, columns);
    display_primary_prompt(git_info, shlvl);
    update_terminal_title(pwd);

    return EXIT_SUCCESS;
}
