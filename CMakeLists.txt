cmake_minimum_required(VERSION 3.1.0)
project(Bootstrap_cpp)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_BUILD_TYPE Debug)
set(LIBRARY_OUTPUT_PATH "Bin")
set(EXECUTABLE_OUTPUT_PATH "Bin")

file(GLOB_RECURSE SOURCES "Source/*.cpp")

add_executable(Bootstrap32 ${SOURCES})
add_executable(Bootstrap64 ${SOURCES})
add_library(Linuxhack32 SHARED "Linuxhack/Appmain.cpp")
add_library(Linuxhack64 SHARED "Linuxhack/Appmain.cpp")

set_target_properties(Bootstrap32 PROPERTIES COMPILE_FLAGS "-m32" LINK_FLAGS "-m32")
set_target_properties(Bootstrap64 PROPERTIES COMPILE_FLAGS "-m64" LINK_FLAGS "-m64")
set_target_properties(Linuxhack32 PROPERTIES COMPILE_FLAGS "-m32" LINK_FLAGS "-m32")
set_target_properties(Linuxhack64 PROPERTIES COMPILE_FLAGS "-m64" LINK_FLAGS "-m64")

target_link_libraries(Bootstrap32 dl)
target_link_libraries(Bootstrap64 dl)
