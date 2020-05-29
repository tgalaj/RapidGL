#include "simple_triangle.h"
#include "input.h"

SimpleTriangle::SimpleTriangle()
{
}

SimpleTriangle::~SimpleTriangle()
{
}

void SimpleTriangle::init_app()
{
    glClearColor(0.5, 0.5, 0.5, 1.0);
}

void SimpleTriangle::input()
{
    if (RapidGL::Input::getKeyUp(RapidGL::KeyCode::Escape))
    {
        stop();
    }
}

void SimpleTriangle::update(double delta_time)
{
}

void SimpleTriangle::render()
{
    glClear(GL_COLOR_BUFFER_BIT);
}

void SimpleTriangle::renderGUI()
{
    RapidGL::CoreApp::renderGUI();
}
