#include "template_project.h"

using namespace RGL;

int main()
{
    std::shared_ptr<CoreApp> app = std::make_shared<TemplateProject>();
    app->init(WINDOW_WIDTH, WINDOW_HEIGHT, "Template Project Demo" /*title*/, 60.0 /*framerate*/);
    app->start();

    return 0;
}