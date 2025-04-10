#include "Internal/Core/App.h"

#include <direct.h>
#include <iostream>

int main()
{
    App::GetInstnace().Init();
    App::GetInstnace().Run();
    App::GetInstnace().Destroy();
    return 0;
}
