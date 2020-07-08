#include <iostream>

#include "simple_triangle.h"

using namespace RapidGL;

int main()
{
    std::shared_ptr<CoreApp> app = std::make_shared<SimpleTriangle>();
    app->init(1280 /*width*/, 720 /*height*/, "Simple Triangle Demo" /*title*/, 60.0 /*framerate*/);
    app->start();

    return 0;
}