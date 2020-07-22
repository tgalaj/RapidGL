#include <iostream>
#include <memory>

#include "toon_outline.h"

using namespace RapidGL;

int main()
{
    std::shared_ptr<CoreApp> app = std::make_shared<ToonOutline>();
    app->init(1280 /*width*/, 720 /*height*/, "Toon Outline Demo" /*title*/, 60.0 /*framerate*/);
    app->start();

    return 0;
}