# Copyright (C) 2022 Tomasz Gałaj

cmake_minimum_required(VERSION 3.21 FATAL_ERROR)
project(RapidGL VERSION 1.0)

set(OpenGL_GL_PREFERENCE GLVND)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include(global_settings)

# ---- Configure the root_directory file ----
configure_file(${CMAKE_SOURCE_DIR}/configuration/root_directory.h.in ${CMAKE_SOURCE_DIR}/configuration/root_directory.h)

# ---- Dependencies ----
add_subdirectory(thirdparty)

# ---- Main project's files ----
add_subdirectory(src)
