#include <memory>

#include "simple_particles_system.h"

using namespace RGL;

int main()
{
    std::shared_ptr<CoreApp> app = std::make_shared<SimpleParticlesSystem>();
    app->init(1280/*width*/, 720/*height*/, "Simple Particles System Demo" /*title*/, 60.0 /*framerate*/);
    app->start();

    return 0;
}