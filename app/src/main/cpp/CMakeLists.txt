cmake_minimum_required(VERSION 3.22.1)
project(mdumper LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# ─── MobileDumper-7 ─────────────────────────────────────────────────────
# Expected to be checked out at app/src/main/cpp/MobileDumper-7/ by CI
# with --recursive (pulls KittyMemoryEx submodule).
# CI also applies our patches from /patches/*.patch.
set(MD7_DIR "${CMAKE_CURRENT_SOURCE_DIR}/MobileDumper-7")

if(NOT EXISTS "${MD7_DIR}/CMakeLists.txt")
    message(FATAL_ERROR "MobileDumper-7 subtree missing at ${MD7_DIR}. "
                        "CI should have cloned it before configuring CMake.")
endif()

# MD7's CMake builds SHARED libMobileDumper-7.so by default.
# DUMPER_BUILD_EXECUTABLE=OFF is default, perfect.
add_subdirectory("${MD7_DIR}" MobileDumper-7)

# ─── Our thin JNI wrapper (just stub functions for the non-MD7 API) ─────
# The real dump call is exported from MD7's main_android.cpp (patched in).
add_library(mdumper_stub SHARED
    jni_stub.cpp
    pubg_walker.cpp
)

target_link_libraries(mdumper_stub
    PRIVATE
    log
    android
)

target_compile_options(mdumper_stub PRIVATE
    -Wall -Wno-unused-parameter -Wno-unused-variable
    $<$<CONFIG:Release>:-O2>
)

# ─── Standalone binary for radar/quick-dump (uses our hand-written walker) ──
add_executable(mdumper_exec
    exec_main.cpp
    pubg_walker.cpp
)
target_link_libraries(mdumper_exec PRIVATE log)
set_target_properties(mdumper_exec PROPERTIES
    OUTPUT_NAME "mdumper_exec"
    PREFIX "lib"
    SUFFIX ".so"
)
target_compile_options(mdumper_exec PRIVATE
    -fPIE
    $<$<CONFIG:Release>:-O2>
)
target_link_options(mdumper_exec PRIVATE -fPIE -pie)
