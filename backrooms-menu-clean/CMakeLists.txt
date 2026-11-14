cmake_minimum_required(VERSION 3.16)
project(BackroomsCleanMenu LANGUAGES C CXX ASM)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(BUILD_EXAMPLES "" CACHE STRING "" FORCE)
# PS2SDK EE headers require _EE before tamtypes.h defines u32/s32/u64.
add_compile_definitions(_EE)
# Ensure typedefs are available before gsKit includes kernel.h.
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -include tamtypes.h")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -include tamtypes.h")
# Add PS2SDK and gsKit headers BEFORE adding gsKit as a subdirectory,
# so its own C sources inherit the required include paths.
include_directories(
  "$ENV{PS2SDK}/common/include"
  "$ENV{PS2SDK}/ee/include"
  "../tyrarooms/vendor/gsKit/ee/gs/include"
  "../tyrarooms/vendor/gsKit/ee/dma/include"
  "../tyrarooms/vendor/gsKit/ee/toolkit/include"
)
add_subdirectory("../tyrarooms/vendor/gsKit" gskit-build)
link_directories("$ENV{PS2SDK}/ee/lib")
add_executable(backrooms_menu_clean src/menu.cpp)
set_target_properties(backrooms_menu_clean PROPERTIES OUTPUT_NAME "backrooms-menu-clean-v2" SUFFIX ".elf")
target_link_libraries(backrooms_menu_clean PRIVATE gskit dmakit pad m)

# Background-only diagnostic ELF: isolates texture upload and GS display.
add_executable(backrooms_background_test src/background_test.cpp)
set_target_properties(backrooms_background_test PROPERTIES OUTPUT_NAME "backrooms-background-test-v3" SUFFIX ".elf")
target_link_libraries(backrooms_background_test PRIVATE gskit dmakit m)

# Previous version targets remain above.

# Six isolated diagnostic variants.
function(add_backrooms_test target output background font ui filter mode)
  add_executable(${target} src/menu_atlas.cpp)
  set_target_properties(${target} PROPERTIES OUTPUT_NAME "${output}" SUFFIX ".elf")
  target_compile_definitions(${target} PRIVATE TEST_BACKGROUND=${background} TEST_FONT=${font} TEST_UI=${ui} TEST_FILTER=${filter} TEST_MODE=${mode})
  target_link_libraries(${target} PRIVATE gskit dmakit pad m)
endfunction()
add_backrooms_test(test01_clear "backrooms-test01-clear-v5" 0 0 0 GS_FILTER_NEAREST GS_MODE_NTSC)
add_backrooms_test(test02_bg "backrooms-test02-background-v5" 1 0 0 GS_FILTER_NEAREST GS_MODE_NTSC)
add_backrooms_test(test03_primitives "backrooms-test03-primitives-v5" 0 0 1 GS_FILTER_NEAREST GS_MODE_NTSC)
add_backrooms_test(test04_font "backrooms-test04-font-v5" 0 1 1 GS_FILTER_NEAREST GS_MODE_NTSC)
add_backrooms_test(test05_bg_linear "backrooms-test05-background-linear-v5" 1 0 0 GS_FILTER_LINEAR GS_MODE_NTSC)
add_backrooms_test(test06_full "backrooms-test06-full-menu-v5" 1 1 1 GS_FILTER_NEAREST GS_MODE_NTSC)


# V6: mesma bateria de seis testes, agora com estado explícito de alpha.
add_backrooms_test(test01_clear_v6 "backrooms-test01-clear-v6" 0 0 0 GS_FILTER_NEAREST GS_MODE_NTSC)
add_backrooms_test(test02_bg_v6 "backrooms-test02-background-v6" 1 0 0 GS_FILTER_NEAREST GS_MODE_NTSC)
add_backrooms_test(test03_primitives_v6 "backrooms-test03-primitives-v6" 0 0 1 GS_FILTER_NEAREST GS_MODE_NTSC)
add_backrooms_test(test04_font_v6 "backrooms-test04-font-v6" 0 1 1 GS_FILTER_NEAREST GS_MODE_NTSC)
add_backrooms_test(test05_bg_linear_v6 "backrooms-test05-background-linear-v6" 1 0 0 GS_FILTER_LINEAR GS_MODE_NTSC)
add_backrooms_test(test06_full_v6 "backrooms-test06-full-menu-v6" 1 1 1 GS_FILTER_NEAREST GS_MODE_NTSC)
# V7: same confirmed V6 menu, with audio loaded from data appended to the ELF.
add_backrooms_test(test06_full_v7 "backrooms-full-menu-v7" 1 1 1 GS_FILTER_NEAREST GS_MODE_NTSC)
target_sources(test06_full_v7 PRIVATE src/audio_stream.cpp)
target_compile_definitions(test06_full_v7 PRIVATE TEST_AUDIO=1)
target_link_libraries(test06_full_v7 PRIVATE audsrv)


# Minimal video initialization probe: no texture/font/menu dependencies.
add_executable(backrooms_launcher_probe src/launcher_probe.cpp)
set_target_properties(backrooms_launcher_probe PROPERTIES OUTPUT_NAME "backrooms-launcher-probe-v1" SUFFIX ".elf")
target_link_libraries(backrooms_launcher_probe PRIVATE gskit dmakit m)
