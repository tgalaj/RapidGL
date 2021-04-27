#include <memory>

#include "vs_disp.h"

using namespace RapidGL;

int main()
{
    std::shared_ptr<CoreApp> app = std::make_shared<VertexDisplacement>();
    app->init(1280/*width*/, 720/*height*/, "Surface animation with vertex displacement Demo" /*title*/, 60.0 /*framerate*/);
    app->start();

    return 0;
}