#include <climits>
#include <cstddef>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>

#include "macros.h"

#if CHAR_BIT != 8
#warning "byte width is not 8; unexpected behaviour may occur"
#endif

/******************************************************************************
 * Store information about a Git repository.
 *****************************************************************************/
class GitRepository
{
public:
    GitRepository(void);
    char const* describe(void);

private:
    void parse_index(void);

private:
    bool started_in_git_dir;
    bool found_git_dir;
};

/******************************************************************************
 * Locate and enter a Git directory in the parent directories of the current
 * working directory.
 *****************************************************************************/
GitRepository::GitRepository()
    : started_in_git_dir(false)
    , found_git_dir(false)
{
    std::filesystem::path current_dir = std::filesystem::current_path();
    std::fprintf(stderr, "current_dir=%ls\n", current_dir.c_str());
    std::filesystem::path root_dir = current_dir.root_path();
    while (true)
    {
        if (current_dir.filename() == ".git")
        {
            this->started_in_git_dir = true;
            return;
        }
        std::filesystem::path git_dir = current_dir / ".git";
        if (std::filesystem::is_directory(git_dir))
        {
            std::filesystem::current_path(git_dir);
            this->found_git_dir = true;
            return;
        }
        if (current_dir == root_dir)
        {
            return;
        }
        current_dir = current_dir.parent_path();
    }
}

/******************************************************************************
 * Obtain information about the Git repository in a form suitable to show in a
 * shell prompt. If required, use a static array or allocate one dynamically to
 * store the information.
 *
 * @return Concise description of the status of the current Git repository.
 *****************************************************************************/
char const* GitRepository::describe(void)
{
    if (this->started_in_git_dir)
    {
        static char git_info[64];
        std::sprintf(git_info, D_YELLOW ".git!" RESET);
        return git_info;
    }
    if (!this->found_git_dir)
    {
        return "";
    }

    // Determine the current ref by reading a file. If the most recent commit
    // is not on a branch, the file will contain its ID. Else, it will contain
    // the symbolic name of the ref.
    std::ifstream head("HEAD");
    std::string line;
    std::getline(head, line);
    char* git_info = new char[line.size()];
    std::size_t slash_idx = line.rfind('/');
    if (slash_idx == std::string::npos)
    {
        std::sprintf(git_info, D_RED "%.7s" RESET, line.data());
    }
    else
    {
        std::sprintf(git_info, D_GREEN "%s" RESET, line.data() + slash_idx + 1);
    }

    this->parse_index();

    return git_info;
}

/******************************************************************************
 * Check whether any files in the repository have been changed.
 *****************************************************************************/
void GitRepository::parse_index(void)
{
    std::ifstream index("index", std::ios::binary);
    index.ignore(8);
    long unsigned entries_size;
    // 044504 041522 000000 001000 000000 042000 113146 036217
    index.read(reinterpret_cast<char*>(&entries_size), 2);
    std::fprintf(stderr, "%lx\n", entries_size);
}

extern "C" char const* get_git_info(void)
{
    return GitRepository().describe();
}
