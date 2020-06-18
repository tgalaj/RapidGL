#include <iostream>
#include <memory>

#include "lighting.h"

using namespace RapidGL;

int main()
{
    std::shared_ptr<CoreApp> app = std::make_shared<Lighting>();
    app->init(800 /*width*/, 600 /*height*/, "Lighting Demo" /*title*/, 60.0 /*framerate*/);
    app->start();

    return 0;
}