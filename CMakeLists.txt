cmake_minimum_required(VERSION 3.1.0)
project(Backend_cpp)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_BUILD_TYPE Debug)
set(EXECUTABLE_OUTPUT_PATH "Bin")

file(GLOB_RECURSE SOURCES "Source/*.cpp")

add_executable(Bootstrap32 ${SOURCES})
add_executable(Bootstrap64 ${SOURCES})

set_target_properties(Bootstrap32 PROPERTIES COMPILE_FLAGS "-m32" LINK_FLAGS "-m32")
set_target_properties(Bootstrap64 PROPERTIES COMPILE_FLAGS "-m64" LINK_FLAGS "-m64")
