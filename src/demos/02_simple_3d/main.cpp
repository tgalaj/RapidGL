#include <iostream>
#include <memory>

#include "simple_3d.h"

using namespace RGL;

int main()
{
    std::shared_ptr<CoreApp> app = std::make_shared<Simple3d>();
    app->init(WINDOW_WIDTH, WINDOW_HEIGHT, "Simple 3D Demo" /*title*/, 60.0 /*framerate*/);
    app->start();

    return 0;
}