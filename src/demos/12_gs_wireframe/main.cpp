#include <memory>

#include "gs_wireframe.h"

using namespace RGL;

int main()
{
    std::shared_ptr<CoreApp> app = std::make_shared<GSWireframe>();
    app->init(1280 /*width*/, 720 /*height*/, "Geometry Shader: Wireframe on top of a shaded model Demo" /*title*/, 60.0 /*framerate*/);
    app->start();

    return 0;
}