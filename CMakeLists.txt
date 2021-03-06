cmake_minimum_required(VERSION 3.1)
project(Bootstrap_cpp)

# Use the latest standard at this time.
set(CMAKE_CXX_STANDARD 17)
enable_language(ASM_MASM)

# Export to the a gitignored directory.
set(EXECUTABLE_OUTPUT_PATH ${Bootstrap_cpp_SOURCE_DIR}/Bin)
set(LIBRARY_OUTPUT_PATH ${Bootstrap_cpp_SOURCE_DIR}/Bin)

# Set warningflags per compiler.
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    if (CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
        string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    else()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
    endif()
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_DEBUG} -g")
endif()

# Separate release and debugbuilds output and dependencies.
mark_as_advanced(CMAKE_IMPORT_LIBRARY_SUFFIX CMAKE_DEBUG_POSTFIX)
set(CMAKE_DEBUG_POSTFIX "d")

# Just pull all the files from /Source
file(GLOB_RECURSE SOURCES "Source/*.cpp")
file(GLOB_RECURSE ASSEMBLY "Source/*.asm")

# Build the requested version without the 'lib' prefix.
if(${CMAKE_SIZEOF_VOID_P} EQUAL 4)
    add_executable(Bootstrap ${SOURCES} ${ASSEMBLY})
    set_target_properties(Bootstrap PROPERTIES SUFFIX "32")
else()
    add_executable(Bootstrap ${SOURCES} ${ASSEMBLY})
    set_target_properties(Bootstrap PROPERTIES SUFFIX "64")
endif()

# Differentiate between x64 and x86 builds.
if (${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU" OR ${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
    if(${CMAKE_SIZEOF_VOID_P} EQUAL 4)
        set_target_properties(Bootstrap PROPERTIES COMPILE_FLAGS "-m32" LINK_FLAGS "-m32 -static-libgcc -static-libstdc++")
    else()
        set_target_properties(Bootstrap PROPERTIES COMPILE_FLAGS "-m64" LINK_FLAGS "-m64 -static-libgcc -static-libstdc++")
    endif()
endif()

# Local library dependencies.
if (${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU" OR ${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
    target_link_libraries(Bootstrap dl pthread ${OPENSSL_LIBRARIES})
else()
    target_link_libraries(Bootstrap ws2_32 ${OPENSSL_LIBRARIES})
endif()

# External library dependencies.
find_package(OPENSSL)
if (OPENSSL_FOUND)
    include_directories(${OPENSSL_INCLUDE_DIR})
    target_link_libraries(Bootstrap ${OPENSSL_LIBRARIES})
endif()

# Use static VC runtimes when releasing on Windows.
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    foreach(flag_var
            CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE
            CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO)
        if(${flag_var} MATCHES "/MD")
            string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
        endif()
    endforeach()
endif()
