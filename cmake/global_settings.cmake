# Copyright (C) 2022 Tomasz Ga³aj

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "CMake")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_DEBUG_POSTFIX "_d")
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)

# Set glfw variables
set(GLFW_BUILD_EXAMPLES OFF CACHE INTERNAL "Build the GLFW example programs")
set(GLFW_BUILD_TESTS    OFF CACHE INTERNAL "Build the GLFW test programs")
set(GLFW_BUILD_DOCS     OFF CACHE INTERNAL "Build the GLFW documentation")
set(GLFW_INSTALL        OFF CACHE INTERNAL "Generate installation target")

# Set Assimp variables
set(BUILD_SHARED_LIBS                     OFF CACHE INTERNAL "Build package with shared libraries.")
set(ASSIMP_NO_EXPORT                      ON  CACHE BOOL "")
set(ASSIMP_BUILD_ASSIMP_TOOLS             OFF CACHE BOOL "")
set(ASSIMP_BUILD_TESTS                    OFF CACHE BOOL "")
set(ASSIMP_INSTALL_PDB                    OFF CACHE BOOL "")
set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF CACHE BOOL "")
set(ASSIMP_BUILD_OBJ_IMPORTER             ON  CACHE BOOL "")
set(ASSIMP_BUILD_GLTF_IMPORTER            ON  CACHE BOOL "")
set(ASSIMP_BUILD_FBX_IMPORTER             ON  CACHE BOOL "")
set(ASSIMP_BUILD_MD5_IMPORTER             ON  CACHE BOOL "")
set(ASSIMP_BUILD_X_IMPORTER               ON  CACHE BOOL "")