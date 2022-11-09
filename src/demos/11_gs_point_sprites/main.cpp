#include <memory>

#include "gs_point_sprites.h"

using namespace RGL;

int main()
{
    std::shared_ptr<CoreApp> app = std::make_shared<GSPointSprites>();
    app->init(WINDOW_WIDTH, WINDOW_HEIGHT, "Geometry Shader: Point Sprites Demo" /*title*/, 60.0 /*framerate*/);
    app->start();

    return 0;
}