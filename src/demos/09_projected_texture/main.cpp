#include <iostream>
#include <memory>

#include "projected_texture.h"

using namespace RGL;

int main()
{
    std::shared_ptr<CoreApp> app = std::make_shared<ProjectedTexture>();
    app->init(WINDOW_WIDTH, WINDOW_HEIGHT, "Projected Texture Demo" /*title*/, 60.0 /*framerate*/);
    app->start();

    return 0;
}