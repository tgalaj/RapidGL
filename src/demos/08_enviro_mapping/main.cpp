#include <iostream>
#include <memory>

#include "enviro_mapping.h"

using namespace RGL;

int main()
{
    std::shared_ptr<CoreApp> app = std::make_shared<EnvironmentMapping>();
    app->init(WINDOW_WIDTH, WINDOW_HEIGHT, "Environment Mapping Demo" /*title*/, 60.0 /*framerate*/);
    app->start();

    return 0;
}