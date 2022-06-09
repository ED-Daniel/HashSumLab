cmake_minimum_required(VERSION 3.20)
project(HashSum)

set(CMAKE_CXX_STANDARD 20)

add_executable(HashSum main.cpp md5.h md5.cpp sha1.h ini.h SumChecker.cpp SumChecker.h)
