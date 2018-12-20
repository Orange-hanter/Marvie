cmake_minimum_required(VERSION 3.11)

project(fatfs)

set(FATFS_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(FATFS_INCLUDE_DIRS ${FATFS_DIR}/src)

add_library(fatfs src/ff.c src/ffunicode.c)
target_include_directories(fatfs PUBLIC ${FATFS_INCLUDE_DIRS})