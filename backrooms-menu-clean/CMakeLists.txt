cmake_minimum_required(VERSION 3.16)
project(BackroomsCleanMenu LANGUAGES C CXX ASM)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(BUILD_EXAMPLES "" CACHE STRING "" FORCE)
add_subdirectory("../tyrarooms/vendor/gsKit" gskit-build)
include_directories("$ENV{PS2SDK}/common/include" "$ENV{PS2SDK}/ee/include"
  "../tyrarooms/vendor/gsKit/ee/gs/include"
  "../tyrarooms/vendor/gsKit/ee/dma/include"
  "../tyrarooms/vendor/gsKit/ee/toolkit/include")
link_directories("$ENV{PS2SDK}/ee/lib")
add_executable(backrooms_menu_clean src/menu.cpp)
set_target_properties(backrooms_menu_clean PROPERTIES OUTPUT_NAME "backrooms-menu-clean-v1" SUFFIX ".elf")
target_link_libraries(backrooms_menu_clean PRIVATE gskit dmakit pad m)
