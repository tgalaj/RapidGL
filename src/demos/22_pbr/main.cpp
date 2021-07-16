#include <iostream>
#include <memory>

#include "pbr.h"

using namespace RGL;

int main()
{
    std::shared_ptr<CoreApp> app = std::make_shared<PBR>();
    app->init(1280/*width*/, 720/*height*/, "Physically Based Rendering Demo" /*title*/, 60.0 /*framerate*/);
    app->start();

    return 0;
}