#include <filesystem>
#include <iostream>

class GitRepository
{
public:
    GitRepository(void);

private:
    void parse(void);
};

GitRepository::GitRepository()
{
    std::filesystem::path directory = std::filesystem::canonical(".");
    std::filesystem::path root_directory = directory.root_path();
    while (true)
    {
        std::filesystem::path git_directory = directory / ".git";
        if (std::filesystem::is_directory(git_directory))
        {
            std::filesystem::current_path(git_directory);
            this->parse();
            break;
        }
        if (directory == root_directory)
        {
            break;
        }
        directory = directory.parent_path();
    }
}

void GitRepository::parse(void){
}

extern "C"
{
    char const *get_git_info(void)
    {
        GitRepository git_repository;
        return "git branch";
    }
}
