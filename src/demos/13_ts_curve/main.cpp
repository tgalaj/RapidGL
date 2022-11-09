#include <memory>

#include "ts_curve.h"

using namespace RGL;

int main()
{
    std::shared_ptr<CoreApp> app = std::make_shared<Tessellation1D>();
    app->init(WINDOW_WIDTH, WINDOW_HEIGHT, "Tessellation - 1D Demo" /*title*/, 60.0 /*framerate*/);
    app->start();

    return 0;
}