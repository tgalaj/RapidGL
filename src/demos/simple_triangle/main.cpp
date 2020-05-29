#include <iostream>

#include "simple_triangle.h"

using namespace RapidGL;

int main()
{
    std::shared_ptr<CoreApp> app = std::make_shared<SimpleTriangle>();
    app->init(800, 600, "Simple Triangle Demo", 60.0);
    app->start();

    return 0;
}