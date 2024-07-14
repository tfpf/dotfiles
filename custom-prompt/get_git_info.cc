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
    std::string get_git_branch(void);

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
    std::string git_branch = this->get_git_branch();
    static char *git_info = new char[git_branch.size() + 64];
    std::sprintf(git_info, "%s", git_branch.data());
    return git_info;
}

/******************************************************************************
 * Determine the current branch of the Git repository.
 *
 * @return Branch name if available, else latest commit ID.
 *****************************************************************************/
std::string GitRepository::get_git_branch(void)
{
    std::ifstream head("HEAD");
    std::string line;
    std::getline(head, line);
    return std::filesystem::path(line).filename().string();
}

extern "C"
{
    char const *get_git_info(void)
    {
        return GitRepository().describe();
    }
}
