cmake_minimum_required(VERSION 3.11)

project(tinyxml2)

set(TINYXML2_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(TINYXML2_INCLUDE_DIRS ${TINYXML2_DIR})

add_library(tinyxml2 tinyxml2.cpp)
target_include_directories(tinyxml2 PUBLIC ${TINYXML2_INCLUDE_DIRS})
