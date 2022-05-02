cmake_minimum_required(VERSION 3.16)
set(DCMAKE_SH="CMAKE_SH-NOTFOUND")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 1)
project(YunGameServer)

add_subdirectory(external/PGUtils)
include_directories(external/PGUtils)

add_definitions(-DPGZXB_DEBUG)
add_definitions(-D__STDC_FORMAT_MACROS)
add_definitions(-Wall)
add_subdirectory(tests)

file(GLOB yun_game_server_src_files src/*.cpp)
add_executable(YunGameServer ${yun_game_server_src_files})
