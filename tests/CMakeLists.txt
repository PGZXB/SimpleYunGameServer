include_directories(../external/PGUtils)

file(GLOB test_src_files *.cpp)
add_executable(YunGameServer_test ${test_src_files})
target_link_libraries(YunGameServer_test PGUtils PGTestMain)
