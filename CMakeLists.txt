cmake_minimum_required(VERSION 3.16)
set(DCMAKE_SH="CMAKE_SH-NOTFOUND")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)
project(YunGameServer)

include_directories(src)

add_subdirectory(external/PGUtils)
include_directories(external/PGUtils)
include_directories(external/ConcurrentQueue)
include_directories(external/ThreadPool)

SET(CMAKE_BUILD_TYPE "Debug")
add_definitions(-DPGZXB_DEBUG)
add_definitions(-DPGYGS_WITH_LOG)
add_definitions(-D__STDC_FORMAT_MACROS)
add_definitions(-Wall)
add_subdirectory(tests)

find_package(Threads)

file(GLOB yun_game_server_src_files src/*.cpp src/**/*.cpp src/**/**/*.cpp)
add_executable(YunGameServer ${yun_game_server_src_files})
target_link_libraries(YunGameServer hv ${CMAKE_THREAD_LIBS_INIT})
