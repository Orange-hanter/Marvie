cmake_minimum_required(VERSION 3.11)

project(_sha1)

set(SHA1_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(SHA1_INCLUDE_DIRS ${SHA1_DIR})

add_library(_sha1 _Sha1.cpp)
target_include_directories(_sha1 PUBLIC ${SHA1_INCLUDE_DIRS})
