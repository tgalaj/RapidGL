#include <iostream>
#include <memory>

#include "alpha_cutout.h"

using namespace RapidGL;

int main()
{
    std::shared_ptr<CoreApp> app = std::make_shared<AlphaCutout>();
    app->init(1280 /*width*/, 720 /*height*/, "Alpha Cutout Demo" /*title*/, 60.0 /*framerate*/);
    app->start();

    return 0;
}