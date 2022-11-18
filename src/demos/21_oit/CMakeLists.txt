# Copyright (C) 2022 Tomasz Gałaj

set(DEMO_NAME "21_oit")

# Add source files
file(GLOB_RECURSE SOURCE_FILES_EXE 
	 ${CMAKE_CURRENT_SOURCE_DIR}/*.c
	 ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

# Add header files
file(GLOB_RECURSE HEADER_FILES_EXE 
	 ${CMAKE_CURRENT_SOURCE_DIR}/*.h
	 ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp)

# Define the executable
add_executable(${DEMO_NAME} ${HEADER_FILES_EXE} ${SOURCE_FILES_EXE})

# Define the include DIRs
get_target_property(CORE_LIB_INCLUDE ${CORE_LIB_NAME} INCLUDE_DIRECTORIES)

target_include_directories(${DEMO_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(${DEMO_NAME} PRIVATE ${CORE_LIB_INCLUDE})

# Define the link libraries
target_link_libraries(${DEMO_NAME} ${CORE_LIB_NAME})

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "sources" FILES ${SOURCE_FILES_EXE})						   
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "headers" FILES ${HEADER_FILES_EXE})