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
