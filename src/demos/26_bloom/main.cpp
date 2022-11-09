#include <iostream>
#include <memory>

#include "bloom.h"

using namespace RGL;

int main()
{
    std::shared_ptr<CoreApp> app = std::make_shared<Bloom>();
    app->init(WINDOW_WIDTH, WINDOW_HEIGHT, "Bloom Demo" /*title*/, 60.0 /*framerate*/);
    app->start();

    return 0;
}