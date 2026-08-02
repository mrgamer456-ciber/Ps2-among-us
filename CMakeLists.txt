cmake_minimum_required(VERSION 3.10)
project(AmongUs)

file(GLOB_RECURSE SOURCES "*.cpp")

add_ps2_executable(amongus ${SOURCES})
