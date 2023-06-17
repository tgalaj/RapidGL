# Copyright (C) 2022 Tomasz Gałaj

set(CORE_LIB_NAME "RapidGLCore" CACHE STRING "Name of the core library")

# Add source files
file(GLOB_RECURSE SOURCE_FILES_CORE 
	 ${CMAKE_CURRENT_SOURCE_DIR}/*.c
	 ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
	 ${CMAKE_CURRENT_SOURCE_DIR}/gui/*.c
	 ${CMAKE_CURRENT_SOURCE_DIR}/gui/*.cpp)
	
# Add header files
file(GLOB_RECURSE HEADER_FILES_CORE
     ${CMAKE_CURRENT_SOURCE_DIR}/*.h
	 ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp
	 ${CMAKE_CURRENT_SOURCE_DIR}/gui/*.h
	 ${CMAKE_CURRENT_SOURCE_DIR}/gui/*.hpp)

# Define the library
add_library(${CORE_LIB_NAME} STATIC ${HEADER_FILES_CORE} ${SOURCE_FILES_CORE})

target_compile_definitions(${CORE_LIB_NAME} PRIVATE GLFW_INCLUDE_NONE)
target_compile_definitions(${CORE_LIB_NAME} PRIVATE LIBRARY_SUFFIX="")

target_include_directories(${CORE_LIB_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
												   ${CMAKE_SOURCE_DIR}/thirdparty
												   ${CMAKE_SOURCE_DIR}/configuration
												   ${glad_SOURCE_DIR}
												   ${stb_image_SOURCE_DIR}
												   ${imgui_SOURCE_DIR})

target_link_libraries(${CORE_LIB_NAME} ${OPENGL_LIBRARIES})
target_link_libraries(${CORE_LIB_NAME} glad)
target_link_libraries(${CORE_LIB_NAME} stb_image)
target_link_libraries(${CORE_LIB_NAME} assimp)
target_link_libraries(${CORE_LIB_NAME} glfw)
target_link_libraries(${CORE_LIB_NAME} imgui)
target_link_libraries(${CORE_LIB_NAME} spdlog)
target_link_libraries(${CORE_LIB_NAME} glm::glm)

if(MinGW)
    target_link_libraries(${CORE_LIB_NAME} bz2)
endif()

if(MSVC)
    target_compile_definitions(${CORE_LIB_NAME} PUBLIC NOMINMAX)
endif()
