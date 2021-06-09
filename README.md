[![Build Status](https://travis-ci.com/Shot511/RapidGL.svg?token=n4xZiEtrtBgRySDkf6zH&branch=master)](https://travis-ci.com/Shot511/RapidGL)

# RapidGL
Framework for rapid OpenGL demos prototyping.

This framework consists of two major parts:
* **Core library** - which is used as a static library for all examples. Source files are located in ```src/core```.
* **Demos**. Source files are located in ```src/demos```.

## How to build
Make sure to clone the repository with one of the following commands (to also clone the dependencies):

```
git clone --recurse-submodules -j8 https://github.com/Shot511/RapidGL.git

OR

git clone --recurse-submodules https://github.com/Shot511/RapidGL.git

OR

git clone --recursive https://github.com/Shot511/RapidGL.git
```

If you've already cloned the repo run the following command in the root directory to also clone dependencies:

```git submodule update --init --recursive```


Then run the following command in the root directory to generate project files:

```
cmake -B build
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
<img src="screenshots/00_template_project.png" width="50%" height="50%" alt="Template Project" />

### Simple Triangle
<img src="screenshots/01_simple_triangle.png" width="50%" height="50%" alt="Simple Triangle" />

### Simple 3D
This demo shows how to create camera, load models, generate primitives using built-in functions and add textures for specific objects.

<img src="screenshots/02_simple_3d.png" width="50%" height="50%" alt="Simple 3D" />

### Lighting
This demo presents implementation of Blinn-Phong shading model for directional, point and spot lights.

<img src="screenshots/03_lighting.png" width="50%" height="50%" alt="Lighting" />

### Multitextured terrain
This demo presents implementation of multitextured terrain. It uses a blend map (for varying X-Z texturing) and slope based texturing (for texturing the slopes).

<img src="screenshots/04_terrain.png" width="50%" height="50%" alt="Multitextured terrain" />

### Toon shading
This demo presents implementation of various toon shading methods (Simple, Advanced, Simple with Rim, Twin Shade) with different outline rendering methods (Stencil, Post-Process).

<img src="screenshots/05_toon_outline.png" width="50%" height="50%" alt="Toon shading" />

### Simple Fog
Implementation of a simple fog rendering. Three modes are available: linear, exp, exp2.

<img src="screenshots/06_simple_fog.png" width="50%" height="50%" alt="Simple Fog" />

### Alpha Cutout
This demo shows implementation of an alpha cutout using fragments discarding.

<img src="screenshots/07_alpha_cutout.png" width="50%" height="50%" alt="Alpha Cutout" />

### Environment mapping
Implementation of dynamic and static environment mapping (light reflection and refraction).

<img src="screenshots/08_enviro_mapping.png" width="50%" height="50%" alt="Environment mapping" />

### Projected texture
Demo presents projecting a texture onto a surface.

<img src="screenshots/09_projected_texture.png" width="50%" height="50%" alt="Projected texture" />

### Postprocessing filters
Negative, edge detection (Sobel operator) and Gaussian blur filters demo.

<img src="screenshots/10_postprocessing_filters.png" width="50%" height="50%" alt="Postprocessing filters" />

### Geometry Shader: Point Sprites
Demo presents generation of quad sprites from points data using Geometry Shader.

<img src="screenshots/11_gs_point_sprites.png" width="50%" height="50%" alt="Geometry Shader: Point Sprites" />

### Geometry Shader: Wireframe on top of a shaded model
<img src="screenshots/12_gs_wireframe.png" width="50%" height="50%" alt="Geometry Shader: Wireframe on top of a shaded model" />

### Tessellation - 1D
<img src="screenshots/13_ts_curve.png" width="50%" height="50%" alt="Tessellation - 1D" />

### Tessellation - 2D
<img src="screenshots/14_ts_quad.png" width="50%" height="50%" alt="Tessellation - 2D" />

### PN Triangles Tessellation with Level of Detail
This demo implements Point-Normal tessellation algorithm (see *main.cpp* for references) with depth based level of detail (NOTE: works for each mesh with vertex normals).

<img src="screenshots/15_ts_lod.png" width="50%" height="50%" alt="PN Triangles Tessellation with Level of Detail" />

### Procedural noise textures
<img src="screenshots/16_noise.png" width="50%" height="50%" alt="Procedural noise textures" />

### Surface animation with vertex displacement
<img src="screenshots/17_vertex_displacement.png" width="50%" height="50%" alt="Surface animation with vertex displacement" />

### Simple particle system using Transform Feedback
Available presets: fountain, fire and smoke.

<img src="screenshots/18_simple_particles_system.png" width="50%" height="50%" alt="Simple particle system using Transform Feedback" />

### Particle system using instanced meshes
TODO

### Particle system with the Compute Shader
TODO

### Shadow Mapping
TODO: Shadow Mapping with PCF, random sampling.

### Mesh skinning
TODO: LBS, DQS.

### OIT
TODO

### PBR
TODO
