cmake_minimum_required(VERSION 3.31.6)
project(Assignment_2)

set(CMAKE_CXX_STANDARD 26)

add_executable(
        Assignment_2
        main.cpp
        BTreeCommon.cpp
        BTreeCreate.cpp
        BTreeDisplay.cpp
        BTreeSearch.cpp
        BTreeInsert.cpp
        BTreeDelete.cpp
        BTreeIndex.h)
