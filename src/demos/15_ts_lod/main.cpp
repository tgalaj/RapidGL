#include <memory>

#include "ts_lod.h"

using namespace RGL;

/**
 * References:
 * Vlachos Alex, Jorg Peters, Chas Boydand Jason L.Mitchell. "Curved PN Triangles".Proceedings of the 2001 Symposium interactive 3D graphics(2001) : 159 - 66.
 * John McDonald. "Tessellation On Any Budget".Game Developers Conference, 2011.
 */
int main()
{
    std::shared_ptr<CoreApp> app = std::make_shared<TessellationLoD>();
    app->init(1280/*width*/, 720/*height*/, "PN Triangles Tessellation with Level of Detail Demo" /*title*/, 60.0 /*framerate*/);
    app->start();

    return 0;
}