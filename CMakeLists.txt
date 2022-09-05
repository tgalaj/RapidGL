# Copyright (C) 2022 Tomasz Gałaj

cmake_minimum_required(VERSION 3.9.2 FATAL_ERROR)
project(RapidGL VERSION 0.1)

set(OpenGL_GL_PREFERENCE GLVND)

set(THIRDPARTY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty")

# add thirdparties
include(thirdparty/thirdparty.cmake)

add_subdirectory(src)
