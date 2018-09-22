cmake_minimum_required(VERSION 3.10)
project(LinearHashing)
include_directories(.)

set(CMAKE_CXX_STANDARD 14)
#set(CMAKE_BUILD_TYPE Debug)

add_executable(LinearHashing main.cpp ADS_set.h)