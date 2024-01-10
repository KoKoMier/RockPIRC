set(RCDriver "spidev" CACHE  INTERNAL "")

#CMake 最低版本要求
cmake_minimum_required (VERSION 2.8)

#项目信息
project (Demo)

set(src_dir main.cpp )
add_executable(RockPiRC ${src_dir})

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(RockPiRC PRIVATE Threads::Threads)