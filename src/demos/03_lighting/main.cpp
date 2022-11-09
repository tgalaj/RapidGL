#include <iostream>
#include <memory>

#include "lighting.h"

using namespace RGL;

int main()
{
    std::shared_ptr<CoreApp> app = std::make_shared<Lighting>();
    app->init(WINDOW_WIDTH, WINDOW_HEIGHT, "Lighting Demo" /*title*/, 60.0 /*framerate*/);
    app->start();

    return 0;
}