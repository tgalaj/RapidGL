#include <iostream>
#include <memory>

#include "toon_outline.h"

using namespace RGL;

int main()
{
    std::shared_ptr<CoreApp> app = std::make_shared<ToonOutline>();
    app->init(WINDOW_WIDTH, WINDOW_HEIGHT, "Toon Outline Demo" /*title*/, 60.0 /*framerate*/);
    app->start();

    return 0;
}