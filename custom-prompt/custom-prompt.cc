#include <algorithm>
#include <chrono>
#include <csignal>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <future>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <utility>

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
#define B_GREEN BEGIN_INVISIBLE ESCAPE LEFT_SQUARE_BRACKET "92m" END_INVISIBLE
#define B_GREEN_RAW ESCAPE LEFT_SQUARE_BRACKET "92m"
#define B_GREY BEGIN_INVISIBLE ESCAPE LEFT_SQUARE_BRACKET "90m" END_INVISIBLE
#define B_GREY_RAW ESCAPE LEFT_SQUARE_BRACKET "90m"
#define B_RED BEGIN_INVISIBLE ESCAPE LEFT_SQUARE_BRACKET "91m" END_INVISIBLE
#define B_RED_RAW ESCAPE LEFT_SQUARE_BRACKET "91m"
#define B_YELLOW BEGIN_INVISIBLE ESCAPE LEFT_SQUARE_BRACKET "93m" END_INVISIBLE

// Dark.
#define D_CYAN_RAW ESCAPE LEFT_SQUARE_BRACKET "36m"
#define D_GREEN BEGIN_INVISIBLE ESCAPE LEFT_SQUARE_BRACKET "32m" END_INVISIBLE
#define D_GREEN_RAW ESCAPE LEFT_SQUARE_BRACKET "32m"
#define D_RED BEGIN_INVISIBLE ESCAPE LEFT_SQUARE_BRACKET "31m" END_INVISIBLE
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

/**
 * Represent an amount of time.
 */
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

/**
 * Initialise from the given amount of time.
 *
 * @param delay Time measured in nanoseconds.
 */
Interval::Interval(long long unsigned delay)
{
    this->milliseconds = (delay /= 1000000ULL) % 1000;
    this->seconds = (delay /= 1000) % 60;
    this->minutes = (delay /= 60) % 60;
    this->hours = delay / 60;
    LOG_DEBUG("Delay is %u h %u m %u s %u ms.", this->hours, this->minutes, this->seconds, this->milliseconds);
}

/**
 * Output the amount of time succinctly.
 *
 * @param ostream Output stream.
 */
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

/**
 * Output the amount of time with units.
 *
 * @param ostream Output stream.
 */
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

/**
 * Get the ID of the currently-focused window.
 *
 * @return Active window ID. On Linux, if there is no X display running, 0 is
 * returned. Likewise, on macOS, if no topmost window is found, 0 is returned.
 */
extern "C" long long unsigned get_active_wid(void);

/**
 * Store information about the current Git repository.
 */
class GitRepository
{
private:
    C::git_repository* repo;
    bool bare, detached;
    std::filesystem::path gitdir;
    C::git_reference* ref;
    C::git_oid const* oid;
    std::string description, tag;
    std::string state;
    unsigned dirty, staged, untracked;

public:
    GitRepository(void);
    std::string get_information(void);

private:
    void establish_description(void);
    void establish_tag(void);
    void establish_state(void);
    void establish_state_rebasing(void);
    void establish_dirty_staged_untracked(void);
    // These are static methods because otherwise, their signatures do not
    // match the required signatures for use as callback functions.
    static int update_tag(char const*, C::git_oid*, void*);
    static int update_dirty_staged_untracked(char const*, unsigned, void*);
};

/**
 * Read the current Git repository.
 */
GitRepository::GitRepository(void) :
    repo(nullptr), bare(false), detached(false), ref(nullptr), oid(nullptr), dirty(0), staged(0), untracked(0)
{
    if (C::git_libgit2_init() <= 0)
    {
        return;
    }
    if (C::git_repository_open_ext(&this->repo, ".", 0, nullptr) != 0)
    {
        return;
    }
    this->bare = C::git_repository_is_bare(this->repo);
    this->detached = C::git_repository_head_detached(this->repo);
    this->gitdir = C::git_repository_path(this->repo);
    this->establish_description();
    this->establish_tag();
    this->establish_state();
    this->establish_dirty_staged_untracked();
}

/**
 * Obtain a human-readable description of the working tree of the current Git
 * repository. This shall be the name of the current branch if it is available.
 * Otherwise, it shall be the hash of the most recent commit.
 */
void GitRepository::establish_description(void)
{
    if (C::git_repository_head(&this->ref, this->repo) == 0)
    {
        // According to the documentation, this retrieves the reference object
        // ID only if the reference is direct. However, I observed that it does
        // so even if the reference is symbolic (i.e. if we are on a branch).
        // There is no harm in leaving it here because if it fails, it will
        // just return a null pointer.
        this->oid = C::git_reference_target(this->ref);

        char const* branch_name;
        if (C::git_branch_name(&branch_name, this->ref) == 0)
        {
            this->description = branch_name;
            return;
        };

        // We are not on a branch. The reference must be direct. Use the commit
        // hash.
        this->description = C::git_oid_tostr_s(this->oid);
        this->description.erase(12);
        return;
    }

    // We must be on a branch with no commits. Obtain the required information
    // manually.
    std::ifstream head_file(this->gitdir / "HEAD");
    if (!head_file.good())
    {
        return;
    }
    std::getline(head_file, this->description);
    if (this->description.rfind("ref: refs/heads/", 0) == 0)
    {
        this->description.erase(0, 16);
    }
}

/**
 * Obtain the tag of the working tree of the current Git repository (if there
 * is one).
 */
void GitRepository::establish_tag(void)
{
    // If a tag or a tagged commit is not checked out (which is the case if we
    // are on a branch), don't search. (This makes the common case fast.) If
    // the most recent commit is not available, there is nothing to search
    // anyway.
    if (!this->detached || this->oid == nullptr)
    {
        return;
    }
    C::git_tag_foreach(this->repo, this->update_tag, this);
}

/**
 * Obtain the state of the working tree of the current Git repository. This
 * shall be the name of the operation currently in progress (if any).
 */
void GitRepository::establish_state(void)
{
    switch (C::git_repository_state(this->repo))
    {
    case C::GIT_REPOSITORY_STATE_BISECT:
        this->state = "bisecting";
        break;
    case C::GIT_REPOSITORY_STATE_CHERRYPICK:
    case C::GIT_REPOSITORY_STATE_CHERRYPICK_SEQUENCE:
        this->state = "cherry-picking";
        break;
    case C::GIT_REPOSITORY_STATE_MERGE:
        this->state = "merging";
        break;
    case C::GIT_REPOSITORY_STATE_REBASE:
    case C::GIT_REPOSITORY_STATE_REBASE_INTERACTIVE:
    case C::GIT_REPOSITORY_STATE_REBASE_MERGE:
        this->establish_state_rebasing();
        break;
    case C::GIT_REPOSITORY_STATE_REVERT:
    case C::GIT_REPOSITORY_STATE_REVERT_SEQUENCE:
        this->state = "reverting";
        break;
    }
}

/**
 * Obtain the rebase state of the working tree of the current Git repository.
 */
void GitRepository::establish_state_rebasing(void)
{
    this->state = "rebasing";
    std::ifstream msgnum_file(this->gitdir / "rebase-merge/msgnum");
    if (!msgnum_file.good())
    {
        return;
    }
    std::ifstream end_file(this->gitdir / "rebase-merge/end");
    if (!end_file.good())
    {
        return;
    }
    std::string msgnum_contents, end_contents;
    if (!(msgnum_file >> msgnum_contents) || !(end_file >> end_contents))
    {
        return;
    }
    this->state += ' ' + msgnum_contents + '/' + end_contents;
}

/**
 * Obtain the statuses of the index and working tree of the current Git
 * repository.
 */
void GitRepository::establish_dirty_staged_untracked(void)
{
    C::git_status_options opts = GIT_STATUS_OPTIONS_INIT;
    opts.flags = C::GIT_STATUS_OPT_INCLUDE_UNTRACKED | C::GIT_STATUS_OPT_EXCLUDE_SUBMODULES;
    C::git_status_foreach_ext(this->repo, &opts, this->update_dirty_staged_untracked, this);
}

/**
 * Check whether the given tag matches the reference of the given
 * `GitRepository` instance. If it does, update the corresponding member of the
 * latter.
 *
 * @param name Tag name.
 * @param oid Tag object ID.
 * @param self_ `GitRepository` instance whose member should be updated.
 *
 * @return 1 if the tag matches the reference, 0 otherwise.
 */
int GitRepository::update_tag(char const* name, C::git_oid* oid, void* self_)
{
    GitRepository* self = static_cast<GitRepository*>(self_);

    // Compare the object ID of the tag with the object ID of the reference.
    if (C::git_oid_cmp(oid, self->oid) != 0)
    {
        C::git_tag* tag;
        if (C::git_tag_lookup(&tag, self->repo, oid) != 0)
        {
            // This is an unannotated tag, meaning that its object ID is the
            // same as the object ID of the corresponding commit. The latter
            // does not match the object ID of the reference.
            return 0;
        }

        // This is an annotated tag, meaning that its object ID is different
        // from the object ID of the corresponding commit. Find the latter and
        // compare it with the object ID of the reference.
        C::git_oid const* oid = C::git_tag_target_id(tag);
        if (C::git_oid_cmp(oid, self->oid) != 0)
        {
            // The latter does not match the object ID of the reference.
            return 0;
        }
    }

    self->tag = name;
    if (self->tag.rfind("refs/tags/", 0) == 0)
    {
        self->tag.erase(0, 10);
    }
    // Found a match. Stop iterating.
    return 1;
}

/**
 * Check whether the given file is modified, staged or untracked. If it is,
 * update the corresponding members of the given `GitRepository` instance.
 *
 * @param _path File path.
 * @param status_flags Flags indicating the status of the file.
 * @param self_ `GitRepository` instance whose members should be updated.
 *
 * @return 1 if all statuses are recorded, 0 otherwise.
 */
int GitRepository::update_dirty_staged_untracked(char const* _path, unsigned status_flags, void* self_)
{
    GitRepository* self = static_cast<GitRepository*>(self_);
    self->dirty |= status_flags
        & (C::GIT_STATUS_WT_DELETED | C::GIT_STATUS_WT_MODIFIED | C::GIT_STATUS_WT_RENAMED
            | C::GIT_STATUS_WT_TYPECHANGE);
    self->staged |= status_flags
        & (C::GIT_STATUS_INDEX_DELETED | C::GIT_STATUS_INDEX_MODIFIED | C::GIT_STATUS_INDEX_NEW
            | C::GIT_STATUS_INDEX_RENAMED | C::GIT_STATUS_INDEX_TYPECHANGE);
    self->untracked |= status_flags & C::GIT_STATUS_WT_NEW;

    // Stop iterating if all possible statuses were found.
    return self->dirty && self->staged && self->untracked;
}

/**
 * Provide information about the current Git repository in a manner suitable to
 * display in the shell prompt.
 *
 * @return Git information.
 */
std::string GitRepository::get_information(void)
{
    if (this->repo == nullptr)
    {
        return "";
    }
    std::ostringstream information_stream;
    if (this->bare)
    {
        information_stream << "bare | ";
    }
    if (this->detached)
    {
        information_stream << D_RED << this->description << RESET;
    }
    else
    {
        information_stream << D_GREEN << this->description << RESET;
    }
    if (!this->tag.empty())
    {
        information_stream << " 󰓼 " << this->tag;
    }
    if (this->dirty || this->staged || this->untracked)
    {
        information_stream << ' ';
    }
    if (this->dirty)
    {
        information_stream << B_YELLOW "*" RESET;
    }
    if (this->staged)
    {
        information_stream << B_GREEN "+" RESET;
    }
    if (this->untracked)
    {
        information_stream << B_RED "!" RESET;
    }
    if (!this->state.empty())
    {
        information_stream << " | " << this->state;
    }
    return information_stream.str();
}

/**
 * Get the current timestamp.
 *
 * @return Time in nanoseconds since a fixed but unspecified reference point.
 */
long long unsigned get_timestamp(void)
{
    std::chrono::time_point now = std::chrono::system_clock::now();
    long long unsigned ts = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
    return ts;
}

/**
 * Show a completed command using a desktop notification.
 *
 * @param last_command Most-recently run command.
 * @param exit_code Code with which the command exited.
 * @param interval Running time of the command.
 */
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

/**
 * Show a completed command textually.
 *
 * @param last_command Most-recently run command.
 * @param exit_code Code with which the command exited.
 * @param interval Running time of the command.
 * @param columns Width of the terminal window.
 */
void write_report(std::string_view const& last_command, int exit_code, Interval const& interval, std::size_t columns)
{
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
        = (sizeof D_CYAN_RAW + sizeof D_GREEN_RAW + 2 * sizeof RESET_RAW - 4) / sizeof(char);
    std::size_t width = columns + multi_byte_correction + non_printing_correction;
    LOG_DEBUG("Padding report to %zu characters.", width);
    std::clog << '\r' << std::setw(width) << report << '\n';
}

/**
 * Show information about the running time of a command if it ran for long.
 *
 * @param last_command Most-recently run command.
 * @param exit_code Code with which the command exited.
 * @param delay Running time of the command in nanoseconds.
 * @param prev_active_wid ID of the focused window when the command started.
 * @param columns Width of the terminal window.
 */
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
    last_command.remove_prefix(last_command.find_first_not_of(' '));
    last_command.remove_suffix(last_command.size() - 1 - last_command.find_last_not_of(' '));

    Interval interval(delay);
    write_report(last_command, exit_code, interval, columns);
    if (delay <= 10000000000ULL)
    {
        return;
    }

    long long unsigned curr_active_wid = get_active_wid();
    LOG_DEBUG("ID of focused window when command started was %llu.", prev_active_wid);
    LOG_DEBUG("ID of focused window when command finished is %llu.", curr_active_wid);
    if (prev_active_wid != curr_active_wid)
    {
        notify_desktop(last_command, exit_code, interval);
    }
}

/**
 * Show the primary prompt.
 *
 * @param shlvl Current shell level.
 * @param git_repository_information_future Git information provider.
 * @param venv Python virtual environment.
 */
void display_primary_prompt(
    int shlvl, std::future<std::string> const& git_repository_information_future, char const* venv)
{
    std::cout << "\n" HOST_ICON " " BBI_YELLOW HOST RESET "  " BB_CYAN DIRECTORY RESET;
    if (git_repository_information_future.wait_for(std::chrono::milliseconds(150)) != std::future_status::ready)
    {
        std::cout << "  " << B_GREY "unavailable" RESET;
    }
    else
    {
        std::string git_repository_information = git_repository_information_future.get();
        if (!git_repository_information.empty())
        {
            std::cout << "  " << git_repository_information;
        }
    }
    if (venv != nullptr)
    {
        std::cout << "  " B_BLUE << venv << RESET;
    }
    std::cout << "\n";
    while (--shlvl > 0)
    {
        std::cout << "▶";
    }
    std::cout << PROMPT_SYMBOL " ";
}

/**
 * Set the title of the current terminal window to the current directory
 * followed by the directory separator, unless the current directory is the
 * Linux/macOS root directory in which case, set the title to just a slash.
 * This should also automatically update the title of the current terminal tab.
 *
 * @param pwd Current directory.
 */
void set_terminal_title(std::string_view& pwd)
{
    LOG_DEBUG("Current directory path is '%s'.", pwd.data());
    pwd.remove_prefix(pwd.rfind('/') + 1);
    std::clog << ESCAPE RIGHT_SQUARE_BRACKET "0;" << pwd << '/' << ESCAPE BACKSLASH;
}

/**
 * Actual entry point. The C++ standard forbids recursively calling `main`, so
 * the program code is written in this function instead.
 *
 * @param argc Number of command line arguments.
 * @param argv Command line arguments.
 *
 * @return Exit code.
 */
int main_internal(int const argc, char const* argv[])
{
    long long unsigned ts = get_timestamp();
    if (argc <= 1)
    {
        std::cout << ts << ' ' << get_active_wid() << '\n';
        return EXIT_SUCCESS;
    }

    // Start another thread to obtain information about the current Git
    // repository.
    std::promise<std::string> git_repository_information_promise;
    std::future<std::string> git_repository_information_future = git_repository_information_promise.get_future();
    std::thread(
        [](std::promise<std::string> git_repository_information_promise)
        {
            git_repository_information_promise.set_value(GitRepository().get_information());
        },
        // I prefer to transfer ownership of the promise to the thread, because
        // it may continue running after the main thread terminates.
        std::move(git_repository_information_promise))
        .detach();

    std::string_view last_command(argv[1]);
    int exit_code = std::stoi(argv[2]);
    long long unsigned delay = ts - std::stoull(argv[3]);
    long long unsigned prev_active_wid = std::stoull(argv[4]);
    std::size_t columns = std::stoull(argv[5]);
    report_command_status(last_command, exit_code, delay, prev_active_wid, columns);

    std::string_view pwd(argv[6]);
    set_terminal_title(pwd);

    int shlvl = std::stoi(argv[7]);
    char const* venv = getenv("VIRTUAL_ENV_PROMPT");
    display_primary_prompt(shlvl, git_repository_information_future, venv);

    return EXIT_SUCCESS;
}

int main(int const argc, char const* argv[])
{
    // Repeated keyboard interrupts cause this program to crash for unclear
    // reasons. Ignore them. It isn't expected to run for long, after all.
    std::signal(SIGINT, SIG_IGN);

    // For testing. Simulate dummy arguments so that the longer code path is
    // taken. Honour the standard requirement that the argument list be
    // null-terminated.
    if (argc == 2)
    {
        char const* argv[] = { "custom-prompt", "[] last_command", "0", "0", "0", "79", "/", "1", nullptr };
        int constexpr argc = sizeof argv / sizeof *argv - 1;
        return main_internal(argc, argv);
    }

    return main_internal(argc, argv);
}
