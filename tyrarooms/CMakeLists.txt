cmake_minimum_required(VERSION 3.16)
project(BackroomsPS2 LANGUAGES C CXX ASM)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(BUILD_EXAMPLES "" CACHE STRING "Do not build upstream gsKit samples" FORCE)

add_subdirectory(vendor/gsKit gskit-build)

add_executable(backrooms src/main.cpp)
set_target_properties(backrooms PROPERTIES SUFFIX ".elf")

target_include_directories(backrooms PRIVATE
    vendor/gsKit/ee/gs/include
    vendor/gsKit/ee/dma/include
)

target_link_libraries(backrooms PRIVATE gskit dmakit m)
