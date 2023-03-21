#include <iostream>
#include <memory>

#include "clustered_shading.h"

using namespace RGL;

int main()
{
    std::shared_ptr<CoreApp> app = std::make_shared<ClusteredShading>();
    app->init(WINDOW_WIDTH, WINDOW_HEIGHT, "Clustered Shading Demo" /*title*/, 60.0 /*framerate*/);
    app->start();

    return 0;
}