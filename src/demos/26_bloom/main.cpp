#include <iostream>
#include <memory>

#include "bloom.h"

using namespace RGL;

int main()
{
    std::shared_ptr<CoreApp> app = std::make_shared<Bloom>();
    app->init(1280/*width*/, 720/*height*/, "Bloom Demo" /*title*/, 60.0 /*framerate*/);
    app->start();

    return 0;
}