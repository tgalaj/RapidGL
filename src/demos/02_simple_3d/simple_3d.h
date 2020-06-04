#pragma once
#include "core_app.h"

class Simple3d : public RapidGL::CoreApp
{
public:
    Simple3d();
    ~Simple3d();

    void init_app()                override;
    void input()                   override;
    void update(double delta_time) override;
    void render()                  override;
    void renderGUI()               override;
};