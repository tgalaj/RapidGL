#include <iostream>
#include <memory>

#include "cascaded_pcss.h"

using namespace RGL;

int main()
{
    std::shared_ptr<CoreApp> app = std::make_shared<CascadedPCSS>();
    app->init(WINDOW_WIDTH, WINDOW_HEIGHT, "Cascaded Shadow Mapping with Percentage Closer Soft Shadows Demo" /*title*/, 60.0 /*framerate*/);
    app->start();

    return 0;
}