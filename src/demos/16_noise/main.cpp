#include <memory>

#include "noise.h"

using namespace RapidGL;

int main()
{
    std::shared_ptr<CoreApp> app = std::make_shared<ProceduralNoise>();
    app->init(1280 /*width*/, 720 /*height*/, "Procedural Noise Textures Demo" /*title*/, 60.0 /*framerate*/);
    app->start();

    return 0;
}