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
    char const *describe(void);

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
 * @return Concise description of the status of the current Git repository.
 *****************************************************************************/
char const *GitRepository::describe(void)
{
    if (this->started_in_git_dir)
    {
        return ".git!";
    }
    if (!this->found_git_dir)
    {
        return "";
    }

    // Determine the current branch by reading a file. If the latest commit is
    // not on a branch, the file will contain its ID. Either way, the contents
    // of the file are what we need.
    std::ifstream head("HEAD");
    std::string line;
    std::getline(head, line);
    // Hack: treat it as a file path to get the branch name if it is a ref or
    // the commit ID otherwise.
    std::string git_branch = std::filesystem::path(line).filename().string();
    static char *git_info = new char[git_branch.size() + 64];
    std::sprintf(git_info, "%s", git_branch.data());
    return git_info;
}

extern "C"
{
    char const *get_git_info(void)
    {
        return GitRepository().describe();
    }
}
