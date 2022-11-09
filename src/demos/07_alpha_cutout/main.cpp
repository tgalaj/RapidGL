#include <iostream>
#include <memory>

#include "alpha_cutout.h"

using namespace RGL;

int main()
{
    std::shared_ptr<CoreApp> app = std::make_shared<AlphaCutout>();
    app->init(WINDOW_WIDTH, WINDOW_HEIGHT, "Alpha Cutout Demo" /*title*/, 60.0 /*framerate*/);
    app->start();

    return 0;
}