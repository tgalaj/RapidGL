[![Build Status](https://travis-ci.com/Shot511/RapidGL.svg?token=n4xZiEtrtBgRySDkf6zH&branch=master)](https://travis-ci.com/Shot511/RapidGL)

# RapidGL
Framework for rapid OpenGL demos prototyping.

This framework consists of two major parts:
* **Core library** - which is used as a static library for all examples. Source files are located in ```src/core```.
* **Demos**. Source files are located in ```src/demos```.

## How to build
Simply run the following commands in the root directory:

```
mkdir build
cd build
cmake ..
```

These will create project with your system-default build system which will allow you to build and run the examples.

## How to add a new demo using Template Project

The following instructions are also located in ```src/demos/00_template_project/template_project.h```.

To begin creating a new demo using RapidGL framework follow these steps:

1) Create new directory in ```src/demos/<your_dir_name>```.
2) Add the following line to ```src/demos/CMakeLists.txt```: ```add_subdirectory(<your_dir_name>)```.
3) Copy contents of ```src/demos/00_template_project``` to ```src/demos/<your_dir_name>```.
4) Change target name of your demo in ```src/demos/<your_dir_name>/CMakeLists.txt``` from ```set(DEMO_NAME "00_template_project")``` to ```set(DEMO_NAME "your_demo_name")```.
5) (Re-)Run CMake

**Notes:** After changing class name from e.g. TemplateProject to something else, update ```main.cpp``` in ```<your_dir_name>``` accordingly.

## Examples
All of the demos are available in ```src/demos```.

### Template Project
![Template Project](screenshots/00_template_project.png)

### Simple Triangle
![Simple Triangle](screenshots/01_simple_triangle.png)

### Simple 3D
This demo shows how to create camera, load models, generate primitives using built-in functions and add textures for specific objects.

![Simple 3D](screenshots/02_simple_3d.png)