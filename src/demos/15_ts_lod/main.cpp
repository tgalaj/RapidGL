#include <memory>

#include "ts_lod.h"

using namespace RapidGL;

int main()
{
    std::shared_ptr<CoreApp> app = std::make_shared<TessellationLoD>();
    app->init(1280 * 2/*width*/, 720 * 2/*height*/, "Tessellation - 3D and Level of Detail Demo" /*title*/, 60.0 /*framerate*/);
    app->start();

    return 0;
}