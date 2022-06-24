cmake_minimum_required(VERSION 3.16)
project(RISC_V_Simulator)

set(CMAKE_CXX_STANDARD 14)

include_directories(src)

add_library(stage OBJECT src/stage.cpp)

add_executable(RISC_V_Simulator src/main.cpp)

target_link_libraries(RISC_V_Simulator stage)