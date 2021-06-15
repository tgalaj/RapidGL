#include <memory>

#include "instanced_particles_cs.h"

using namespace RGL;

int main()
{
    std::shared_ptr<CoreApp> app = std::make_shared<InstancedParticlesCS>();
    app->init(1280/*width*/, 720/*height*/, "Particle system using instanced meshes with the Compute Shader Demo" /*title*/, 60.0 /*framerate*/);
    app->start();

    return 0;
}