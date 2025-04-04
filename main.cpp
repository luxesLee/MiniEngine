#include "Internal/Core/App.h"

#include <direct.h>
#include <iostream>

int main()
{
    char cwd[1024];
    _getcwd(cwd, sizeof(cwd));
    std::cout << "Current working directory: " << cwd << std::endl;

    App::GetInstnace().Init();
    App::GetInstnace().Run();
    App::GetInstnace().Destroy();
    return 0;
}
