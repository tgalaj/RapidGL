#include <iostream>
#include <memory>

#include "template_project.h"

using namespace RapidGL;

int main()
{
    std::shared_ptr<CoreApp> app = std::make_shared<TemplateProject>();
    app->init(800 /*width*/, 600 /*height*/, "Template Project Demo" /*title*/, 60.0 /*framerate*/);
    app->start();

    return 0;
}