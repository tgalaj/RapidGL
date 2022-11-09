#include <iostream>
#include <memory>

#include "pcss.h"

using namespace RGL;

int main()
{
    std::shared_ptr<CoreApp> app = std::make_shared<PCSS>();
    app->init(WINDOW_WIDTH, WINDOW_HEIGHT, "Percentage Closer Soft Shadows Demo" /*title*/, 60.0 /*framerate*/);
    app->start();

    return 0;
}