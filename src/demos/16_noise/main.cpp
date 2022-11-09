#include <memory>

#include "noise.h"

using namespace RGL;

int main()
{
    std::shared_ptr<CoreApp> app = std::make_shared<ProceduralNoise>();
    app->init(WINDOW_WIDTH, WINDOW_HEIGHT, "Procedural Noise Textures Demo" /*title*/, 60.0 /*framerate*/);
    app->start();

    return 0;
}