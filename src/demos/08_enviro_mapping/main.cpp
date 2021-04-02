#include <iostream>
#include <memory>

#include "enviro_mapping.h."

using namespace RapidGL;

int main()
{
    std::shared_ptr<CoreApp> app = std::make_shared<EnvironmentMapping>();
    app->init(1280 /*width*/, 720 /*height*/, "Environment Mapping Demo" /*title*/, 60.0 /*framerate*/);
    app->start();

    return 0;
}