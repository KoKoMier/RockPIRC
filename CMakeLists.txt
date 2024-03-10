
cmake_minimum_required (VERSION 2.8)
project (RockPiRC)

add_subdirectory(src/M10QGPS)
add_subdirectory(src/QMC5883)
add_subdirectory(src/CRSF)
add_subdirectory(src/MTF-02)

set(RockPiRC main.cpp )
add_executable(RockPiRC ${RockPiRC})

target_link_libraries(RockPiRC ${M10QGPS})
target_link_libraries(RockPiRC ${QMC5883})
target_link_libraries(RockPiRC ${CRSF})
target_link_libraries(RockPiRC ${MTF-02})