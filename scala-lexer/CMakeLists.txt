cmake_minimum_required(VERSION 3.13)
project(cc_labs)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 14)

add_executable(scala_lex main.c lexer.c)
