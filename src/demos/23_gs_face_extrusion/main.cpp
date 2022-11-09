#include <iostream>
#include <memory>

#include "gs_face_extrusion.h"

using namespace RGL;

int main()
{
    std::shared_ptr<CoreApp> app = std::make_shared<GSFaceExtrusion>();
    app->init(WINDOW_WIDTH, WINDOW_HEIGHT, "Geometry Shader: Face extrusion" /*title*/, 60.0 /*framerate*/);
    app->start();

    return 0;
}