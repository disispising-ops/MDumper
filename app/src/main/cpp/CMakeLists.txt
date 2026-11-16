cmake_minimum_required(VERSION 3.22.1)
project(mdumper LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(MD7_DIR "${CMAKE_CURRENT_SOURCE_DIR}/MobileDumper-7")
if(NOT EXISTS "${MD7_DIR}/CMakeLists.txt")
    message(FATAL_ERROR "MobileDumper-7 missing")
endif()
set(DUMPER_ROOT "${MD7_DIR}" CACHE PATH "MD7 root" FORCE)

# Force-include header path
set(FORCE_INC "${CMAKE_CURRENT_SOURCE_DIR}/md7_force_includes.h")

add_subdirectory("${MD7_DIR}" MobileDumper-7)
# Add the pre-include to MD7 target so library build gets it too
target_compile_options(MobileDumper-7 PRIVATE "SHELL:$<$<COMPILE_LANGUAGE:CXX>:-include ${FORCE_INC}>")

# ─── MD7 as standalone executable ──────────────────────────────────
set(KITTYMEM_PATH "${MD7_DIR}/libs/KittyMemoryEx/KittyMemoryEx")
file(GLOB KITTYMEM_SRC "${KITTYMEM_PATH}/*.cpp")
file(GLOB_RECURSE MD7_SRC
    "${MD7_DIR}/Dumper/*.cpp"
    "${MD7_DIR}/Dumper/*.c"
)
list(FILTER MD7_SRC EXCLUDE REGEX ".*MemoryiOS.*")
list(FILTER MD7_SRC EXCLUDE REGEX ".*ProfileiOS.*")
list(FILTER MD7_SRC EXCLUDE REGEX ".*/main_ios\\.mm$")
list(FILTER MD7_SRC EXCLUDE REGEX ".*/UI/iOS/.*")

add_executable(mdumper_full ${MD7_SRC} ${KITTYMEM_SRC})
target_include_directories(mdumper_full PRIVATE "${KITTYMEM_PATH}/..")
target_compile_definitions(mdumper_full PRIVATE
    FMT_HEADER_ONLY kNO_KEYSTONE DUMPER_BUILD_EXECUTABLE NDEBUG)
target_compile_options(mdumper_full PRIVATE
    -stdlib=libc++
    "SHELL:$<$<COMPILE_LANGUAGE:CXX>:-include ${FORCE_INC}>"
    -fexceptions -fPIE
    -Wno-unused-parameter -Wno-unused-function -Wno-missing-field-initializers
    -O2 -fvisibility=hidden -fvisibility-inlines-hidden)
target_link_options(mdumper_full PRIVATE -fPIE -pie -s -static-libstdc++ -Wl,--exclude-libs,ALL)
target_link_libraries(mdumper_full PRIVATE log)
set_target_properties(mdumper_full PROPERTIES
    OUTPUT_NAME "mdumper_full" PREFIX "lib" SUFFIX ".so")

# JNI stub
add_library(mdumper_stub SHARED jni_stub.cpp pubg_walker.cpp)
target_link_libraries(mdumper_stub PRIVATE log android)

# Quick walker (static libc++)
add_executable(mdumper_exec exec_main.cpp pubg_walker.cpp)
target_link_libraries(mdumper_exec PRIVATE log)
set_target_properties(mdumper_exec PROPERTIES
    OUTPUT_NAME "mdumper_exec" PREFIX "lib" SUFFIX ".so")
target_compile_options(mdumper_exec PRIVATE -fPIE -stdlib=libc++)
target_link_options(mdumper_exec PRIVATE
    -fPIE -pie -static-libstdc++ -Wl,--exclude-libs,ALL)
