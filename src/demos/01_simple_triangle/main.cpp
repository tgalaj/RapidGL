#include <iostream>

#include "simple_triangle.h"

using namespace RGL;

int main()
{
    std::shared_ptr<CoreApp> app = std::make_shared<SimpleTriangle>();
    app->init(WINDOW_WIDTH, WINDOW_HEIGHT, "Simple Triangle Demo" /*title*/, 60.0 /*framerate*/);
    app->start();

    return 0;
}