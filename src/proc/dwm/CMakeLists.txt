cmake_minimum_required(VERSION 3.10)

project(dwm)

file(GLOB_RECURSE HEADERS dwm.h ../../include/window.h)
add_executable(dwm dwm.cpp ../../lib/window.cpp ../epoch.cpp ../../include/window.h)

target_include_directories(dwm PUBLIC ./ ../../include ../../kernel/include ../include)