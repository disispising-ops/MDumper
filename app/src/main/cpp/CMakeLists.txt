cmake_minimum_required(VERSION 3.22.1)
project(mdumper LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(MD7_DIR "${CMAKE_CURRENT_SOURCE_DIR}/MobileDumper-7")
if(NOT EXISTS "${MD7_DIR}/CMakeLists.txt")
    message(FATAL_ERROR "MobileDumper-7 missing")
endif()
set(DUMPER_ROOT "${MD7_DIR}" CACHE PATH "MD7 root" FORCE)

add_subdirectory("${MD7_DIR}" MobileDumper-7)

add_library(mdumper_stub SHARED jni_stub.cpp pubg_walker.cpp)
target_link_libraries(mdumper_stub PRIVATE log android)

# Standalone binary — use static libc++ so it doesn't need libc++_shared.so at runtime
add_executable(mdumper_exec exec_main.cpp pubg_walker.cpp)
target_link_libraries(mdumper_exec PRIVATE log)
set_target_properties(mdumper_exec PROPERTIES
    OUTPUT_NAME "mdumper_exec" PREFIX "lib" SUFFIX ".so")
target_compile_options(mdumper_exec PRIVATE -fPIE -stdlib=libc++)
target_link_options(mdumper_exec PRIVATE
    -fPIE -pie
    -static-libstdc++
    -Wl,--exclude-libs,ALL
)
