cmake_minimum_required(VERSION 3.16)
project(BackroomsPS2 LANGUAGES C CXX ASM)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(BUILD_EXAMPLES "" CACHE STRING "Do not build upstream gsKit samples" FORCE)

include_directories(
    "$ENV{PS2SDK}/common/include"
    "$ENV{PS2SDK}/ee/include"
)
add_compile_definitions(_EE)

add_subdirectory(vendor/gsKit gskit-build)

# PS2SDK EE system libraries (including libpad) live here in the toolchain image.
link_directories("$ENV{PS2SDK}/ee/lib")

add_executable(backrooms src/main.cpp)
set_target_properties(backrooms PROPERTIES SUFFIX ".elf")

# Same game and graphics, but skip the title/menu loop for direct gameplay testing.
add_executable(backrooms_nomenu src/main.cpp)
target_compile_definitions(backrooms_nomenu PRIVATE BACKROOMS_SKIP_MENU=1)
set_target_properties(backrooms_nomenu PROPERTIES OUTPUT_NAME "backrooms-nomenu" SUFFIX ".elf")

# Diagnostic build: same maze geometry, but no texture allocation or sampling.
add_executable(backrooms_solid src/main.cpp)
target_compile_definitions(backrooms_solid PRIVATE BACKROOMS_SKIP_MENU=1 BACKROOMS_SOLID_TEST=1)
set_target_properties(backrooms_solid PROPERTIES OUTPUT_NAME "backrooms-solid-test" SUFFIX ".elf")

# Minimal graphics-only diagnostic: no game assets or 3D renderer.
add_executable(gs_diagnostic src/gs_diagnostic.cpp)
set_target_properties(gs_diagnostic PROPERTIES SUFFIX ".elf")

# Second diagnostic reproduces the game's GS setup, but draws only blue.
add_executable(gs_diagnostic_gameinit src/gs_diagnostic_gameinit.cpp)
set_target_properties(gs_diagnostic_gameinit PROPERTIES SUFFIX ".elf")

foreach(target backrooms backrooms_nomenu backrooms_solid gs_diagnostic gs_diagnostic_gameinit)
    target_include_directories(${target} PRIVATE
        vendor/gsKit/ee/gs/include
        vendor/gsKit/ee/dma/include
        vendor/gsKit/ee/toolkit/include
    )
    target_link_libraries(${target} PRIVATE gskit dmakit pad m)
endforeach()
