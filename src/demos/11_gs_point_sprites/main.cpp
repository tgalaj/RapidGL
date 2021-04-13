#include <memory>

#include "gs_point_sprites.h"

using namespace RapidGL;

int main()
{
    std::shared_ptr<CoreApp> app = std::make_shared<GSPointSprites>();
    app->init(1280 /*width*/, 720 /*height*/, "Geometry Shader: Point Sprites Demo" /*title*/, 60.0 /*framerate*/);
    app->start();

    return 0;
}