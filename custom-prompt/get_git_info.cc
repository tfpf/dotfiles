#include <cstddef>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

/******************************************************************************
 * Store information about a Git repository.
 *****************************************************************************/
class GitRepository
{
public:
    GitRepository(void);
    char const *describe(char const *, char const *, char const *);

private:
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
 * Obtain information suitable for use in a shell prompt.
 *
 * @param begin_good_colour Code to set a good foreground colour.
 * @param begin_bad_colour Code to set a bad foreground colour.
 * @param end_colour Code to reset the foreground colour.
 *
 * @return Concise description of the status of the current Git repository.
 *****************************************************************************/
char const *GitRepository::describe(char const *begin_good_colour, char const *begin_bad_colour, char const *end_colour)
{
    if (this->started_in_git_dir)
    {
        static char git_info[16];
        std::sprintf(git_info, "%s.git!%s", begin_good_colour, end_colour);
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
    static char *git_info = new char[line.size()];
    std::size_t slash_idx = line.rfind('/');
    if (slash_idx != std::string::npos)
    {
        std::sprintf(git_info, "%s%s%s", begin_good_colour, line.data() + slash_idx + 1, end_colour);
    }
    else
    {
        std::sprintf(git_info, "%s%.7s%s", begin_bad_colour, line.data(), end_colour);
    }
    return git_info;
}

extern "C"
{
    char const *get_git_info(char const *begin_good_colour, char const *begin_bad_colour, char const *end_colour)
    {
        return GitRepository().describe(begin_good_colour, begin_bad_colour, end_colour);
    }
}
