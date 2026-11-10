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

# Isolated depth diagnostic: same solid maze, with 16-bit reversed Z values.
add_executable(backrooms_solid_reversed_z src/main.cpp)
target_compile_definitions(backrooms_solid_reversed_z PRIVATE BACKROOMS_SKIP_MENU=1 BACKROOMS_SOLID_TEST=1 BACKROOMS_REVERSED_Z_TEST=1)
set_target_properties(backrooms_solid_reversed_z PROPERTIES OUTPUT_NAME "backrooms-solid-reversed-z-test" SUFFIX ".elf")

# Full game build using a 24-bit reversed-Z buffer as a targeted graphics repair.
add_executable(backrooms_z24_repair src/main.cpp)
target_compile_definitions(backrooms_z24_repair PRIVATE BACKROOMS_Z24_REPAIR_V1=1)
set_target_properties(backrooms_z24_repair PROPERTIES OUTPUT_NAME "backrooms-z24-repair-v1" SUFFIX ".elf")

# Minimal graphics-only diagnostic: no game assets or 3D renderer.
add_executable(gs_diagnostic src/gs_diagnostic.cpp)
set_target_properties(gs_diagnostic PROPERTIES SUFFIX ".elf")

# Second diagnostic reproduces the game's GS setup, but draws only blue.
add_executable(gs_diagnostic_gameinit src/gs_diagnostic_gameinit.cpp)
set_target_properties(gs_diagnostic_gameinit PROPERTIES SUFFIX ".elf")

# Isolated 2D-only GS test: no game renderer, textures, or 3D primitives.
add_executable(gs_isolation_01 src/gs_isolation_01.cpp)
set_target_properties(gs_isolation_01 PROPERTIES OUTPUT_NAME "backrooms-isolamento-01" SUFFIX ".elf")

foreach(target backrooms backrooms_nomenu backrooms_solid backrooms_solid_reversed_z backrooms_z24_repair gs_diagnostic gs_diagnostic_gameinit gs_isolation_01)
    target_include_directories(${target} PRIVATE
        vendor/gsKit/ee/gs/include
        vendor/gsKit/ee/dma/include
        vendor/gsKit/ee/toolkit/include
    )
    target_link_libraries(${target} PRIVATE gskit dmakit pad m)
endforeach()

# Clean rebuild stage 01: GS clear only, no menu or game renderer.
add_executable(backrooms_rebuild_01 src/rebuild_01_clear_only.cpp)
set_target_properties(backrooms_rebuild_01 PROPERTIES OUTPUT_NAME "backrooms-rebuild-01-clear-only" SUFFIX ".elf")
target_include_directories(backrooms_rebuild_01 PRIVATE
    vendor/gsKit/ee/gs/include
    vendor/gsKit/ee/dma/include
    vendor/gsKit/ee/toolkit/include
)
target_link_libraries(backrooms_rebuild_01 PRIVATE gskit dmakit pad m)

# Clean rebuild stage 02: one untextured 2D rectangle on the stage 01 base.
add_executable(backrooms_rebuild_02 src/rebuild_02_one_rectangle.cpp)
set_target_properties(backrooms_rebuild_02 PROPERTIES OUTPUT_NAME "backrooms-rebuild-02-one-rectangle" SUFFIX ".elf")
target_include_directories(backrooms_rebuild_02 PRIVATE
    vendor/gsKit/ee/gs/include
    vendor/gsKit/ee/dma/include
    vendor/gsKit/ee/toolkit/include
)
target_link_libraries(backrooms_rebuild_02 PRIVATE gskit dmakit pad m)

# Clean rebuild stage 03: one flat-color, untextured Backrooms room.
add_executable(backrooms_rebuild_03 src/rebuild_03_untextured_room.cpp)
set_target_properties(backrooms_rebuild_03 PROPERTIES OUTPUT_NAME "backrooms-rebuild-03-untextured-room" SUFFIX ".elf")
target_include_directories(backrooms_rebuild_03 PRIVATE
    vendor/gsKit/ee/gs/include
    vendor/gsKit/ee/dma/include
    vendor/gsKit/ee/toolkit/include
)
target_link_libraries(backrooms_rebuild_03 PRIVATE gskit dmakit pad m)

# Clean rebuild stage 04: untextured controllable character with basic walk animation.
add_executable(backrooms_rebuild_04 src/rebuild_04_untextured_player.cpp)
set_target_properties(backrooms_rebuild_04 PROPERTIES OUTPUT_NAME "backrooms-rebuild-04-untextured-player" SUFFIX ".elf")
target_include_directories(backrooms_rebuild_04 PRIVATE
    vendor/gsKit/ee/gs/include
    vendor/gsKit/ee/dma/include
    vendor/gsKit/ee/toolkit/include
)
target_link_libraries(backrooms_rebuild_04 PRIVATE gskit dmakit pad m)

# Clean rebuild stage 05: deeper segmented 3D room with the controllable player.
add_executable(backrooms_rebuild_05 src/rebuild_05_deep_room_player.cpp)
set_target_properties(backrooms_rebuild_05 PROPERTIES OUTPUT_NAME "backrooms-rebuild-05-deep-room-player" SUFFIX ".elf")
target_include_directories(backrooms_rebuild_05 PRIVATE
    vendor/gsKit/ee/gs/include
    vendor/gsKit/ee/dma/include
    vendor/gsKit/ee/toolkit/include
)
target_link_libraries(backrooms_rebuild_05 PRIVATE gskit dmakit pad m)


# Clean rebuild stage 06: first-person corridor, animated hands, long lit room.
add_executable(backrooms_rebuild_06 src/rebuild_06_first_person_long_lit.cpp)
set_target_properties(backrooms_rebuild_06 PROPERTIES OUTPUT_NAME "backrooms-rebuild-06-first-person-lit" SUFFIX ".elf")
target_include_directories(backrooms_rebuild_06 PRIVATE
    vendor/gsKit/ee/gs/include
    vendor/gsKit/ee/dma/include
    vendor/gsKit/ee/toolkit/include
)
target_link_libraries(backrooms_rebuild_06 PRIVATE gskit dmakit pad m)
