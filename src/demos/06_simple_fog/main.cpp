#include <iostream>
#include <memory>

#include "simple_fog.h"

using namespace RGL;

int main()
{
    std::shared_ptr<CoreApp> app = std::make_shared<SimpleFog>();
    app->init(WINDOW_WIDTH, WINDOW_HEIGHT, "Simple Fog Demo" /*title*/, 60.0 /*framerate*/);
    app->start();

    return 0;
}