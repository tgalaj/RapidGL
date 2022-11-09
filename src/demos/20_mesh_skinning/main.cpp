#include <iostream>
#include <memory>

#include "mesh_skinning.h"

using namespace RGL;

int main()
{
    std::shared_ptr<CoreApp> app = std::make_shared<MeshSkinning>();
    app->init(WINDOW_WIDTH, WINDOW_HEIGHT, "Mesh Skinning Demo" /*title*/, 60.0 /*framerate*/);
    app->start();

    return 0;
}