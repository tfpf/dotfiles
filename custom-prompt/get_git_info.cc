#include <iostream>

extern "C"
{
    char const *get_git_info(void)
    {
        return "git branch";
    }
}
