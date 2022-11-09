#include <memory>

#include "vs_disp.h"

using namespace RGL;

int main()
{
    std::shared_ptr<CoreApp> app = std::make_shared<VertexDisplacement>();
    app->init(WINDOW_WIDTH, WINDOW_HEIGHT, "Surface animation with vertex displacement Demo" /*title*/, 60.0 /*framerate*/);
    app->start();

    return 0;
}