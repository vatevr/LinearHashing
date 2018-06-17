cmake_minimum_required(VERSION 3.11)
project(LinearHashing)
include_directories(.)
set(CMAKE_CXX_STANDARD 11)

add_executable(LinearHashing main.cpp ADS_set.h)