cmake_minimum_required(VERSION 3.10)
project(MiraViewer VERSION 1.0)

# C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/headers
    ${CMAKE_SOURCE_DIR}/common/includes
    ${CMAKE_SOURCE_DIR}/common/includes/glad
    ${CMAKE_SOURCE_DIR}/common/includes/GLFW
    ${CMAKE_SOURCE_DIR}/common/includes/glm
    ${CMAKE_SOURCE_DIR}/common/includes/assimp
    ${CMAKE_SOURCE_DIR}/common/includes/imgui
    ${CMAKE_SOURCE_DIR}/common/includes/imguifiledialog
    ${CMAKE_SOURCE_DIR}/common/includes/stb
)

# Link directories
link_directories(
    ${CMAKE_SOURCE_DIR}/common/lib
)

# Find OpenGL
find_package(OpenGL REQUIRED)

# Find all source files
file(GLOB_RECURSE SOURCE_FILES 
    "${CMAKE_SOURCE_DIR}/src/*.cpp"
)

# Find all header files
file(GLOB_RECURSE HEADER_FILES 
    "${CMAKE_SOURCE_DIR}/headers/*.h"
    "${CMAKE_SOURCE_DIR}/headers/*.hpp"
)

# Add GLAD source
list(APPEND SOURCE_FILES "${CMAKE_SOURCE_DIR}/third-party-source-code/glad.c")

# Set up the executable
add_executable(${PROJECT_NAME} ${SOURCE_FILES} ${HEADER_FILES})

# Link libraries
target_link_libraries(${PROJECT_NAME}
    ${OPENGL_LIBRARIES}
    glfw3
    assimp
)

# Set compiler flags
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -pedantic)
endif()

# Copy resource files to build directory
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/shaders $<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/models $<TARGET_FILE_DIR:${PROJECT_NAME}>/models
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/textures $<TARGET_FILE_DIR:${PROJECT_NAME}>/textures
)

# Installation
install(TARGETS ${PROJECT_NAME} DESTINATION bin)
install(DIRECTORY 
    ${CMAKE_SOURCE_DIR}/shaders
    ${CMAKE_SOURCE_DIR}/models
    ${CMAKE_SOURCE_DIR}/textures
    DESTINATION bin
)

# Print configuration information
message(STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
message(STATUS "Build target: ${PROJECT_NAME}")

