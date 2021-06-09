#pragma once
#include "core_app.h"

/** ## Documentation - Creating new project with template_project
  * 
  * To begin creating a new demo using RapidGL framework
  * follow these steps:
  * 1) Create new directory in src/demos/<your_dir_name>
  * 2) Add the following line to src/demos/CMakeLists.txt:
  *    add_subdirectory(<your_dir_name>)
  * 3) Copy contents of src/demos/00_template_project to
  *    src/demos/<your_dir_name>
  * 4) Change target name of your demo in 
  *    src/demos/<your_dir_name>/CMakeLists.txt
  *    from set(DEMO_NAME "00_template_project")
  *    to   set(DEMO_NAME "your_demo_name")
  * 5) (Re-)Run CMake
  *
  * ## Notes:
  *
  * After changing class name from e.g. TemplateProject
  * to something else, update main.cpp accordingly.
  *
**/

class TemplateProject : public RGL::CoreApp
{
public:
    TemplateProject();
    ~TemplateProject();

    void init_app()                override;
    void input()                   override;
    void update(double delta_time) override;
    void render()                  override;
    void render_gui()               override;
};