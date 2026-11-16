cmake_minimum_required(VERSION 3.22.1)
project(mdumper LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# JNI library (links into the app directly for API calls that don't need root)
add_library(mdumper SHARED
    jni_bridge.cpp
    pubg_walker.cpp
)

target_link_libraries(mdumper
    PRIVATE
    log
    android
)

# "Executable" built as a shared library so Android extracts it with the APK
# but has a C main entrypoint. Named libmdumper_exec.so; Kotlin runs it as
# `su -c /path/to/libmdumper_exec.so <outDir>`.
#
# The ld expects this to be a shared lib by extension, but having a main()
# with -pie makes it also runnable as a dyn executable (PIE). We build it
# as an executable and give it a .so-ish name via output properties.
add_executable(mdumper_exec
    exec_main.cpp
    pubg_walker.cpp
)

target_link_libraries(mdumper_exec
    PRIVATE
    log
)

# Make the executable land as libmdumper_exec.so so APK extraction keeps
# it in the nativeLibraryDir with executable bit set.
set_target_properties(mdumper_exec PROPERTIES
    OUTPUT_NAME "mdumper_exec"
    PREFIX "lib"
    SUFFIX ".so"
)

target_compile_options(mdumper PRIVATE
    -Wall -Wno-unused-parameter -Wno-unused-variable
    $<$<CONFIG:Release>:-O2>
)

target_compile_options(mdumper_exec PRIVATE
    -Wall -Wno-unused-parameter -Wno-unused-variable
    -fPIE
    $<$<CONFIG:Release>:-O2>
)

target_link_options(mdumper_exec PRIVATE -fPIE -pie)
