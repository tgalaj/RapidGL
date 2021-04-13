#include <iostream>
#include <memory>

#include "postprocessing_filters.h"

using namespace RapidGL;

int main()
{
    std::shared_ptr<CoreApp> app = std::make_shared<PostprocessingFilters>();
    app->init(1280 /*width*/, 720 /*height*/, "Postprocessing Filters Demo" /*title*/, 60.0 /*framerate*/);
    app->start();

    return 0;
}