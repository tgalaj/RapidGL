#include <memory>

#include "ts_curve.h"

using namespace RGL;

int main()
{
    std::shared_ptr<CoreApp> app = std::make_shared<Tessellation1D>();
    app->init(1280 /*width*/, 720 /*height*/, "Tessellation - 1D Demo" /*title*/, 60.0 /*framerate*/);
    app->start();

    return 0;
}