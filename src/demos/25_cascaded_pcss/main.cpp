#include <iostream>
#include <memory>

#include "cascaded_pcss.h"

using namespace RGL;

int main()
{
    std::shared_ptr<CoreApp> app = std::make_shared<CascadedPCSS>();
    app->init(1280/*width*/, 720/*height*/, "Cascaded Shadow Mapping with Percentage Closer Soft Shadows Demo" /*title*/, 60.0 /*framerate*/);
    app->start();

    return 0;
}