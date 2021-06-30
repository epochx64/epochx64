cmake_minimum_required(VERSION 3.10)

project(terminal)

file(GLOB_RECURSE HEADERS ../../include/window.h)
add_executable(terminal terminal.cpp)

target_include_directories(terminal PUBLIC ./ ../../include ../../kernel/include ../include)