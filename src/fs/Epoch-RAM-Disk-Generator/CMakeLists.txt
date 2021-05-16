cmake_minimum_required(VERSION 3.16)
project(Epoch_RAM_Disk_Generator)

set(CMAKE_CXX_STANDARD 14)

add_executable(Epoch_RAM_Disk_Generator main.cpp ../ext2.cpp)

target_include_directories(Epoch_RAM_Disk_Generator PUBLIC ../../../src/kernel/include ../../../src/include)