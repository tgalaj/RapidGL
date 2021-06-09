#include <iostream>
#include <memory>

#include "projected_texture.h"

using namespace RGL;

int main()
{
    std::shared_ptr<CoreApp> app = std::make_shared<ProjectedTexture>();
    app->init(1280 /*width*/, 720 /*height*/, "Projected Texture Demo" /*title*/, 60.0 /*framerate*/);
    app->start();

    return 0;
}