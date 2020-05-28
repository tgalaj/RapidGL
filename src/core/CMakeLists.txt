# Copyright (C) 2020 Tomasz Gałaj

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
set_property(TARGET ${CORE_LIB_NAME} PROPERTY CXX_STANDARD 17)

# Define the include DIRs
configure_file(${CMAKE_SOURCE_DIR}/configuration/root_directory.h.in ${CMAKE_SOURCE_DIR}/configuration/root_directory.h)
target_include_directories(${CORE_LIB_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/configuration)

target_include_directories(${CORE_LIB_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_include_directories(${CORE_LIB_NAME} PUBLIC "${ASSIMP_INCLUDE_DIR}")
target_include_directories(${CORE_LIB_NAME} PUBLIC "${GLFW_INCLUDE_DIR}")
target_include_directories(${CORE_LIB_NAME} PUBLIC "${GLAD_INCLUDE_DIR}")
target_include_directories(${CORE_LIB_NAME} PUBLIC "${GLM_INCLUDE_DIR}")
target_include_directories(${CORE_LIB_NAME} PUBLIC "${IMGUI_INCLUDE_DIR}")
target_include_directories(${CORE_LIB_NAME} PUBLIC "${STB_IMAGE_INCLUDE_DIR}")

target_link_libraries(${CORE_LIB_NAME} "${OPENGL_LIBRARY}")
target_link_libraries(${CORE_LIB_NAME} "${ASSIMP_LIBRARY}")
target_link_libraries(${CORE_LIB_NAME} "${GLFW_LIBRARY}")
target_link_libraries(${CORE_LIB_NAME} "${GLAD_LIBRARY}"      "${CMAKE_DL_LIBS}")
target_link_libraries(${CORE_LIB_NAME} "${IMGUI_LIBRARY}"     "${CMAKE_DL_LIBS}")
target_link_libraries(${CORE_LIB_NAME} "${STB_IMAGE_LIBRARY}" "${CMAKE_DL_LIBS}")

target_compile_definitions(${CORE_LIB_NAME} PRIVATE GLFW_INCLUDE_NONE)
target_compile_definitions(${CORE_LIB_NAME} PRIVATE LIBRARY_SUFFIX="")

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}   PREFIX "sources"   FILES ${SOURCE_FILES_CORE})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}   PREFIX "headers"   FILES ${HEADER_FILES_CORE})