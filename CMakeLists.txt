cmake_minimum_required(VERSION 3.28)
project(debufrizer)

set(CMAKE_CXX_STANDARD 23)

add_executable(debufrizer
        main.cpp
        section0.cpp
        section1.cpp
        descriptor.cpp
        section3and4.cpp
	code_table.cpp
)
