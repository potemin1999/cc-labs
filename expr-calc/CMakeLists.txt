cmake_minimum_required(VERSION 3.13)
project(cc_labs)

set(CMAKE_CXX_STANDARD 17)

add_executable(expr_calc Calculator.cpp Main.cpp)
target_link_libraries(expr_calc stdc++)