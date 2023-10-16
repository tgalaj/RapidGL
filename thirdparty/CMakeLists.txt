# CPM - package manager
include(CPM)

# OpenGL
find_package(OpenGL REQUIRED)

# glad
set(GLAD_DIR ${CMAKE_CURRENT_LIST_DIR}/glad)
set(glad_SOURCE_DIR ${GLAD_DIR}/include CACHE INTERNAL "")
add_library(glad STATIC ${GLAD_DIR}/src/glad.c)
target_include_directories(glad PRIVATE ${GLAD_DIR}/include)

# stb_image
set(STB_IMAGE_DIR ${CMAKE_CURRENT_LIST_DIR}/stb_image)
set(stb_image_SOURCE_DIR ${STB_IMAGE_DIR} CACHE INTERNAL "")
add_library(stb_image STATIC ${STB_IMAGE_DIR}/stb_image.cpp 
						     ${STB_IMAGE_DIR}/stb_image_write.cpp 
							 ${STB_IMAGE_DIR}/stb_image_resize.cpp)
target_include_directories(stb_image PRIVATE ${STB_IMAGE_DIR})

# other
CPMAddPackage("gh:assimp/assimp@5.2.5")
CPMAddPackage("gh:glfw/glfw#3.3.8")
CPMAddPackage("gh:g-truc/glm#0.9.9.8")
CPMAddPackage("gh:ocornut/imgui@1.88")
CPMAddPackage("gh:gabime/spdlog@1.10.0")

CPMAddPackage(
        GITHUB_REPOSITORY benikabocha/tinyddsloader
        GIT_TAG master
        GIT_SHALLOW TRUE
        DOWNLOAD_ONLY TRUE)

if (tinyddsloader_ADDED)
    add_library(tinyddsloader INTERFACE)
    target_include_directories(tinyddsloader INTERFACE "${tinyddsloader_SOURCE_DIR}")
endif()

set(imgui_SOURCE_DIR ${imgui_SOURCE_DIR} CACHE INTERNAL "")
add_library(imgui STATIC ${imgui_SOURCE_DIR}/imgui.cpp
					     ${imgui_SOURCE_DIR}/imgui_demo.cpp
					     ${imgui_SOURCE_DIR}/imgui_draw.cpp
						 ${imgui_SOURCE_DIR}/imgui_tables.cpp
					     ${imgui_SOURCE_DIR}/imgui_widgets.cpp)
						 
set_target_properties(glad
                      stb_image 
                      assimp 
                      glfw 
                      glm 
                      imgui 
                      spdlog
                      tinyddsloader PROPERTIES FOLDER "thirdparty")

if (TARGET zlibstatic)
    set_target_properties(zlibstatic PROPERTIES FOLDER "thirdparty")
endif()