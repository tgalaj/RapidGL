#pragma once

#include "core_app.h"

class SimpleTriangle : public RapidGL::CoreApp
{
public:
    SimpleTriangle();
    ~SimpleTriangle();

    virtual void init_app()                override;
    virtual void input()                   override;
    virtual void update(double delta_time) override;
    virtual void render()                  override;
    virtual void renderGUI()               override;
};