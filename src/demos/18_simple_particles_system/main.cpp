#include <memory>

#include "simple_particles_system.h"

using namespace RGL;

int main()
{
    std::shared_ptr<CoreApp> app = std::make_shared<SimpleParticlesSystem>();
    app->init(WINDOW_WIDTH, WINDOW_HEIGHT, "Simple Particles System Demo" /*title*/, 60.0 /*framerate*/);
    app->start();

    return 0;
}