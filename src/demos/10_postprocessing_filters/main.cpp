#include <iostream>
#include <memory>

#include "postprocessing_filters.h"

using namespace RGL;

int main()
{
    std::shared_ptr<CoreApp> app = std::make_shared<PostprocessingFilters>();
    app->init(WINDOW_WIDTH, WINDOW_HEIGHT, "Postprocessing Filters Demo" /*title*/, 60.0 /*framerate*/);
    app->start();

    return 0;
}