#include <iostream>
#include <memory>

#include "simple_fog.h"

using namespace RapidGL;

int main()
{
    std::shared_ptr<CoreApp> app = std::make_shared<SimpleFog>();
    app->init(1280 /*width*/, 720 /*height*/, "Simple Fog Demo" /*title*/, 60.0 /*framerate*/);
    app->start();

    return 0;
}