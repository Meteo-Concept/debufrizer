cmake_minimum_required(VERSION 3.25)
project(debufrizer)

set(CMAKE_CXX_STANDARD 17)

find_package(GDAL REQUIRED)

add_executable(debufrizer
        main.cpp
        section0.cpp
        section1.cpp
        descriptor.cpp
        section3and4.cpp
        code_table.cpp
)

target_link_libraries(debufrizer PRIVATE GDAL::GDAL)
