cmake_minimum_required(VERSION 3.22.1)
project(mdumper LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# ─── MobileDumper-7 ─────────────────────────────────────────────────────
# Bundled as a subtree. Build it as a static library so we link one .so.
set(DUMPER_BUILD_EXECUTABLE OFF CACHE BOOL "" FORCE)
set(DUMPER_BUILD_LIBRARY    ON  CACHE BOOL "" FORCE)
add_subdirectory(MobileDumper-7 EXCLUDE_FROM_ALL)

# ─── Our JNI wrapper ────────────────────────────────────────────────────
add_library(mdumper SHARED
    jni_bridge.cpp
)

target_link_libraries(mdumper
    PRIVATE
    MobileDumper-7
    log
    android
)

target_compile_options(mdumper PRIVATE
    -Wall -Wno-unused-parameter -Wno-unused-variable
    $<$<CONFIG:Release>:-O2>
)
