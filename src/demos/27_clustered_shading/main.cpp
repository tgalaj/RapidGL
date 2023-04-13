#include <iostream>
#include <memory>

#include "clustered_shading.h"

using namespace RGL;

int main()
{
    std::shared_ptr<CoreApp> app = std::make_shared<ClusteredShading>();
    app->init(1920, 1080, "Clustered Shading Demo" /*title*/, 6000.0 /*framerate*/);
    app->start();

    return 0;
}