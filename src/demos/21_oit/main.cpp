#include <iostream>
#include <memory>

#include "oit.h"

using namespace RGL;

int main()
{
    std::shared_ptr<CoreApp> app = std::make_shared<OIT>();
    app->init(1280 /*width*/, 720 /*height*/, "Order Independent Transparency Demo" /*title*/, 60.0 /*framerate*/);
    app->start();

    return 0;
}