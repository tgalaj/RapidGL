#include <iostream>
#include <memory>

#include "terrain.hpp"

using namespace RGL;

int main()
{
    std::shared_ptr<CoreApp> app = std::make_shared<Terrain>();
    app->init(WINDOW_WIDTH, WINDOW_HEIGHT, "Terrain Demo" /*title*/, 60.0 /*framerate*/);
    app->start();

    return 0;
}